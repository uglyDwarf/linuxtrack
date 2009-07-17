/*************************************************************
 ****************** CAMERA ABSTRACTION LAYER *****************
 *************************************************************/
#include <stdbool.h>
#include <stdio.h>
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

