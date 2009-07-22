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

#include "cal.h"
#include "tir4_driver.h"
#include "wiimote_driver.h"

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
  int returnval;

  switch (ccb->device.category) {
  case tir4_camera:
    returnval = tir4_init(ccb);
    break;
  case webcam:
/*     returnval = webcam_init(ccb); */
    break;
  case wiimote:
#ifdef CWIID
     returnval = wiimote_init(ccb);
#else
/*     returnval = wiimote_init(ccb); */
#endif
    break;
  }
  return returnval;
}

int cal_shutdown(struct camera_control_block *ccb)
{
  switch (ccb->device.category) {
  case tir4_camera:
    return tir4_shutdown(ccb);
    break;
  case webcam:
/*     return webcam_shutdown(ccb); */
    break;
  case wiimote:
#ifdef CWIID
     return wiimote_shutdown(ccb); 
#else
/*     return wiimote_shutdown(ccb); */
#endif
    break;
  }
  return -1;
}

int cal_suspend(struct camera_control_block *ccb)
{
  switch (ccb->device.category) {
  case tir4_camera:
    return tir4_suspend(ccb);
    break;
  case webcam:
/*     return webcam_suspend(ccb); */
    break;
  case wiimote:
#ifdef CWIID
     return wiimote_suspend(ccb);
#else
/*     return wiimote_suspend(ccb); */
#endif 
    break;
  }
  return -1;
}

void cal_change_operating_mode(struct camera_control_block *ccb,
                              enum cal_operating_mode newmode)
{
  switch (ccb->device.category) {
  case tir4_camera:
    tir4_change_operating_mode(ccb,newmode);
    break;
  case webcam:
/*     return webcam_change_operating_mode(ccb,newmode); */
    break;
  case wiimote:
#ifdef CWIID
    wiimote_change_operating_mode(ccb,newmode); 
#else
/*    wiimote_change_operating_mode(ccb,newmode); */
#endif
    break;
  }
}

int cal_wakeup(struct camera_control_block *ccb)
{
  switch (ccb->device.category) {
  case tir4_camera:
    return tir4_wakeup(ccb);
    break;
  case webcam:
/*     return webcam_wakeup(ccb); */
    break;
  case wiimote:
#ifdef CWIID
     return wiimote_wakeup(ccb); 
#else
/*     return wiimote_wakeup(ccb); */
#endif
    break;
  }
  return -1;
}

int cal_set_good_indication(struct camera_control_block *ccb,
                             bool arg)
{
  switch (ccb->device.category) {
  case tir4_camera:
    return tir4_set_good_indication(ccb, arg);
    break;
  case webcam:
    /* do nothing */
    break;
  case wiimote:
    /* do nothing (?) */
    break;
  }
  return -1;
}

int cal_get_frame(struct camera_control_block *ccb,
                         struct frame_type *f)
{
  switch (ccb->device.category) {
  case tir4_camera:
    return tir4_get_frame(ccb, f);
    break;
  case webcam:
/*     return webcam_populate_frame(ccb, f); */
    break;
  case wiimote:
#ifdef CWIID
     return wiimote_get_frame(ccb, f); 
#else
/*     return wiimote_get_frame(ccb, f); */
#endif
    break;
  }
  return -1;
}

void frame_free(struct camera_control_block *ccb,
                struct frame_type *f)
{
  free(f->bloblist.blobs);
  if (ccb->mode == diagnostic) {
    free(f->bitmap);
  }
}

void frame_print(struct frame_type f)
{
  printf("-- start frame --\n");
  printf("num blobs: %d\n", f.bloblist.num_blobs);
  bloblist_print(f.bloblist);
  /* FIXME: print something for pixels? */
  printf("-- end frame --\n");
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


void blob_print(struct blob_type b)
{
  printf("x: %f\ty: %f\tscore: %d\n", b.x,b.y,b.score);
}

/* Thread implementation*/
pthread_t capture_thread_id;

//Those are last data from hardware (processed)
pthread_mutex_t pending_frame_mutex = PTHREAD_MUTEX_INITIALIZER;
/* guarded by pending_frame_mutex: { */
static struct frame_type pending_frame;
static bool pending_frame_valid = false;
/* } */

pthread_mutex_t capture_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
/* guarded by capture_thread_mutex: { */
static bool pending_capture_state_active = false;
static bool current_capture_state_active = false;
/* } */

/* Start the capture thread, this WILL BLOCK until the 
 * thread has actually started; this should be a very short 
 * period */
void cal_thread_start(struct camera_control_block *ccb)
{
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
    return;
  }
  /* so no thread is running, we can access all shared
   * variables freely right now */
  pending_frame_valid = false;
  current_capture_state_active = false;
  pending_capture_state_active = true;
  int res = pthread_create(&capture_thread_id, NULL, 
                           capture_thread, (void*)ccb);
  if(res != 0){
    fprintf(stderr, "Can't create capture thread!\n");
    exit(1);
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
void cal_thread_get_frame(struct frame_type *return_frame, 
                          bool *return_frame_valid)
{
  if(pthread_mutex_trylock(&pending_frame_mutex) == 0){
    //we hold the lock now!
    if (pending_frame_valid) {
      *return_frame_valid = true;
      pending_frame_valid = false;
      *return_frame = pending_frame;
    }
    else {
      *return_frame_valid = false;
    }
    pthread_mutex_unlock(&pending_frame_mutex);
  }
  else {
    *return_frame_valid = false;
  }
}


//Main function of capture thread
void *capture_thread(void *ccb)
{
  struct frame_type frame;
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
      cal_get_frame(ccb, &frame);
      pthread_mutex_lock(&pending_frame_mutex);
      /* if there is an unread valid frame,
       * we free it before overwriting it */
      if (pending_frame_valid) {
        frame_free(ccb, &pending_frame);
      }
      pending_frame_valid = true;
      pending_frame = frame;
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
