/*************************************************************
 ****************** CAMERA ABSTRACTION LAYER *****************
 *************************************************************/
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "cal.h"
#include "pose.h"
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
     returnval = wiimote_init(ccb); 
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
     return wiimote_shutdown(ccb); 
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
     return wiimote_suspend(ccb); 
    break;
  }
  return -1;
}

void cal_change_operating_mode(struct camera_control_block *ccb,
                               enum cal_operating_mode newmode)
{
  switch (ccb->device.category) {
  case tir4_camera:
    return tir4_change_operating_mode(ccb,newmode);
    break;
  case webcam:
/*     return webcam_change_operating_mode(ccb,newmode); */
    break;
  case wiimote:
     return wiimote_change_operating_mode(ccb,newmode); 
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
     return wiimote_wakeup(ccb); 
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
     return wiimote_get_frame(ccb, f); 
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

//Helper function that allows me to copy transform between different sourcs
void copy_transform(struct transform *src, struct transform *dest)
{
  int i,j;
  for(i = 0; i < 3; ++i){
    dest->tr[i] = src->tr[i];
  }

  for(i = 0; i < 3; ++i){
    for(j = 0; i<3; ++i){
      dest->rot[i][j] = src->rot[i][j];
    }
  }
  
}


pthread_t capture_thread_id;
//Setting this to false couses thread exit loop 
volatile bool capture_stop_flag = false;

//Those are last data from hardware (processed)
struct transform last_transform;
pthread_mutex_t last_data_mutex = PTHREAD_MUTEX_INITIALIZER;


//Main function of capture thread
void *capture_thread(void *ccb)
{
  struct frame_type frame;
  struct transform t;
  while(capture_stop_flag != true){
    cal_get_frame(ccb, &frame);
    pose_process_blobs(frame.bloblist, &t);

    pthread_mutex_lock(&last_data_mutex);
    copy_transform(&t, &last_transform);
    pthread_mutex_unlock(&last_data_mutex);
    frame_free(ccb, &frame);
  }
  capture_stop_flag = false;
  return NULL;
}

//Access function meant to be used from outside
void get_current_transform(struct transform *res)
{
  static struct transform data = {
    .tr = {0.0f, 0.0f, 0.0f},
    .rot = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}
  };
  
  /*
  If we get the mutex, we are good to go and read the data...
  If not, just output last data.
  */
  if(pthread_mutex_trylock(&last_data_mutex) == 0){
    //we hold the lock now!
    copy_transform(&last_transform, &data);
    pthread_mutex_unlock(&last_data_mutex);
  }
  copy_transform(&data, res);
}

//Start the capture thread
bool thread_start()
{
  //Check if there is not already request to stop...
  if(capture_stop_flag == true){
    return false;
  }

  capture_stop_flag = false;
  int res = pthread_create(&capture_thread_id, NULL, 
                           capture_thread, (void*)NULL);
  if(res != 0){
    printf("Can't create capture thread!\n");
    return false;
  }
  return true;
}

//Request termination of capture thread
void thread_stop()
{
  capture_stop_flag = true;
}
