/*************************************************************
 ****************** CAMERA ABSTRACTION LAYER *****************
 *************************************************************/
#ifdef HAVE_CONFIG_H
  #include "../config.h"
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#include "cal.h"
#include "tir4_driver.h"
#include "tir_driver.h"
#include "wiimote_driver.h"
#include "webcam_driver.h"
#include "utils.h"


dev_interface *iface = NULL;



/*********************/
/* private Constants */
/*********************/
/* none */

/**************************/
/* private Static members */
/**************************/
/* none */

void *capture_thread(void *ccb);

/************************/
/* function definitions */
/************************/
int cal_init(struct camera_control_block *ccb)
{
  assert(ccb != NULL);
  switch (ccb->device.category) {
    case tir:
#if HAVE_LIBUSB_1_0 || HAVE_LIBOPENUSB
      iface = &tir_interface;
#else
      log_message("Track IR driver was not compiled!\n");
#endif
      break;
    case tir4_camera:
#ifdef HAVE_LIBUSB
      iface = &tir4_interface;
#else
      log_message("Track IR4 driver was not compiled!\n");
#endif
      break;
    case webcam:
#ifdef V4L2
      iface = &webcam_interface;
#else
      log_message("Webcam driver was not compiled!\n");
#endif
      break;
    case wiimote:
#ifdef HAVE_LIBCWIID
      iface = &wiimote_interface;
#else
      log_message("Wiimote driver was not compiled!\n");
#endif
      break;
    default:
      assert(0);
      break;
  }
  if(iface == NULL){
    log_message("Driver not found!\n");
    return -1;
  }
  assert((iface != NULL) && (iface->device_init != NULL));
  return (iface->device_init)(ccb);
}

int cal_shutdown(struct camera_control_block *ccb)
{
  assert(ccb != NULL);
  assert((iface != NULL) && (iface->device_shutdown != NULL));
  
  return (iface->device_shutdown)(ccb);
}

int cal_suspend(struct camera_control_block *ccb)
{
  assert(ccb != NULL);
  assert((iface != NULL) && (iface->device_suspend != NULL));
  return (iface->device_suspend)(ccb);
}

void cal_change_operating_mode(struct camera_control_block *ccb,
                              enum cal_operating_mode newmode)
{
  assert(ccb != NULL);
  assert((iface != NULL) && (iface->device_change_operating_mode != NULL));
  (iface->device_change_operating_mode)(ccb, newmode);
  return;
}

int cal_wakeup(struct camera_control_block *ccb)
{
  assert(ccb != NULL);
  assert((iface != NULL) && (iface->device_wakeup != NULL));
  return (iface->device_wakeup)(ccb);
}

int cal_set_good_indication(struct camera_control_block *ccb,
                             bool arg)
{
  assert(ccb != NULL);
  assert(iface != NULL);
  if(iface->device_set_good_indication != NULL){
    return (iface->device_set_good_indication)(ccb, arg);
  }else{
    return 0;
  }
}

int cal_get_frame(struct camera_control_block *ccb,
                         struct frame_type *f)
{
  assert(ccb != NULL);
  assert(f != NULL);
  assert((iface != NULL) && (iface->device_get_frame != NULL));
  return (iface->device_get_frame)(ccb, f);
}

void frame_free(struct camera_control_block *ccb,
                struct frame_type *f)
{
  assert(ccb != NULL);
  assert(f->bloblist.blobs != NULL);
  free(f->bloblist.blobs);
  f->bloblist.blobs = NULL;
  if (ccb->mode == diagnostic) {
    assert(f->bitmap != NULL);
    free(f->bitmap);
    f->bitmap = NULL;
  }
}

void blob_print(struct blob_type b)
{
  printf("x: %f\ty: %f\tscore: %d\n", b.x,b.y,b.score);
}

void bloblist_print(struct bloblist_type bl)
{
  int i;

  printf("-- start blob --\n");
  for (i=0;i<bl.num_blobs;i++) {
    blob_print((bl.blobs)[i]);
  }
  printf("-- end blob --\n");
}

void frame_print(struct frame_type f)
{
  printf("-- start frame --\n");
  printf("num blobs: %d\n", f.bloblist.num_blobs);
  bloblist_print(f.bloblist);
  /* FIXME: print something for pixels? */
  printf("-- end frame --\n");
}


/* Thread implementation*/
pthread_t capture_thread_id;

//Those are last data from hardware (processed)
pthread_mutex_t pending_frame_mutex = PTHREAD_MUTEX_INITIALIZER;
/* guarded by pending_frame_mutex: { */
static struct frame_type pending_frame;
static bool pending_frame_valid = false;
static int pending_frame_error = 0;
/* } */

pthread_mutex_t capture_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
/* guarded by capture_thread_mutex: { */
static bool pending_capture_state_active = false;
static bool current_capture_state_active = false;
/* } */

/* Start the capture thread, this WILL BLOCK until the 
 * thread has actually started; this should be a very short 
 * period */
int cal_thread_start(struct camera_control_block *ccb)
{
  assert(ccb != NULL);
  /* check if there is a capture thread running, 
   * and handle this oddball case:
   *   - capture thread was previously started, 
   *   - then it was told to stop, however,
   *     - there were no frames, so its hung, unable to actually stop.
   *   - we have called it to start again, so lets just clear the stop 
   *     signal and return.
   * In the typo case of starting the thread twice, well,
   * we'll just continue running*/
  bool already_running = false;
  pthread_mutex_lock(&capture_thread_mutex);
  if(current_capture_state_active){
    already_running = true;
    pending_capture_state_active = true;
  }
  pthread_mutex_unlock(&capture_thread_mutex);
  if (already_running) {
    return 0;
  }
  /* so no thread is running, we can access all shared
   * variables freely right now */
  pending_frame_error = 0;
  pending_frame_valid = false;
  current_capture_state_active = false;
  pending_capture_state_active = true;
  int res = pthread_create(&capture_thread_id, NULL, 
                           capture_thread, (void*)ccb);
  if(res != 0){
    log_message("Can't create capture thread!\n");
    return -1;
  }
  /* thread created, to keep things simple, we block until
   * it has started executing */ 
  bool done = false;
  while (!done) {
    pthread_mutex_lock(&capture_thread_mutex);
    if(current_capture_state_active){
      done = true;
    }
    pthread_mutex_unlock(&capture_thread_mutex);
  }
  return 0;
}

/* Request termination of capture thread
 * note that the thread may not actually stop if its 
 * hung up looking for a frame.  It will stop when the 
 * it gets a chance though.  use the cal_thread_is_stopped()
 * below to poll until its really stopped, if needed */ 
void cal_thread_stop(void)
{
  /* tell it to die */
  pthread_mutex_lock(&capture_thread_mutex);
  pending_capture_state_active = false;
  pthread_mutex_unlock(&capture_thread_mutex);
}

/* returns true if the thread is actually stopped */
bool cal_thread_is_stopped(void)
{
  /* did it actually die? */
  bool it_died=false;
  pthread_mutex_lock(&capture_thread_mutex);
  if(current_capture_state_active == false){
    it_died = true;
  }
  pthread_mutex_unlock(&capture_thread_mutex);
  return it_died;
}

/* Access function meant to be used from outside
 * return_frame is assumed to be a freed frame, 
 * (no bitmap or blobs allocated). 
 * be sure to free the frame returned when done with it! */
int cal_thread_get_frame(struct frame_type *return_frame, 
                         bool *return_frame_valid)
{
  int retval=0;
  if(pthread_mutex_trylock(&pending_frame_mutex) == 0){
    //we hold the lock now!
    if (pending_frame_error < 0) {
      pending_frame_valid = false;
      retval = pending_frame_error;
    }
    else {
      if (pending_frame_valid) {
        *return_frame_valid = true;
	//this causes frame to be processed only one time
        pending_frame_valid = false;
        *return_frame = pending_frame;
      }
      else {
        *return_frame_valid = false;
      }
    }
    pthread_mutex_unlock(&pending_frame_mutex);
  }
  else {
    *return_frame_valid = false;
  }
  return retval;
}


//Main function of capture thread
void *capture_thread(void *ccb)
{
  struct frame_type frame;
  int retval;
  /* on entry, we signal that we are running */
  pthread_mutex_lock(&capture_thread_mutex);
  current_capture_state_active = true;
  pthread_mutex_unlock(&capture_thread_mutex);
  /* the run until told to stop loop */
  bool terminate_flag = false;
  while (!terminate_flag) {
    /* watch for a signal to flag termination */
    pthread_mutex_lock(&capture_thread_mutex);
    if(pending_capture_state_active == false){
      terminate_flag = true;
    }
    pthread_mutex_unlock(&capture_thread_mutex);
    if (!terminate_flag) {
      retval = cal_get_frame(ccb, &frame);
      pthread_mutex_lock(&pending_frame_mutex);
      /* if there is an unread valid frame,
       * we free it before overwriting it */
      if (pending_frame_valid) {
        frame_free(ccb, &pending_frame);
      }
      if (retval < 0) {
        pending_frame_valid = false;
      }
      else {
        pending_frame_valid = true;
        pending_frame = frame;
      }
      pending_frame_error = retval;
      pthread_mutex_unlock(&pending_frame_mutex);
    }
  }
  pthread_mutex_lock(&pending_frame_mutex);
  /* if there is final unread valid frame,
   * we free it before terminating */
  if (pending_frame_valid) {
    frame_free(ccb, &pending_frame);
  }
  pending_frame_valid = false;
  pthread_mutex_unlock(&pending_frame_mutex);

  pthread_mutex_lock(&capture_thread_mutex);
  current_capture_state_active = false;
  pthread_mutex_unlock(&capture_thread_mutex);
  return NULL;
}
