#include <stdio.h>
#include "ltlib.h"

/**************************/
/* private Static members */
/**************************/
static struct camera_control_block ccb;
static float filterfactor=1.0;
static float angle_scalefactor=1.0;
static struct bloblist_type filtered_bloblist;
static struct blob_type filtered_blobs[3];
static bool first_frame_read = false;

/*******************************/
/* private function prototypes */
/*******************************/
float expfilt(float x, 
              float y_minus_1,
              float filtfactor);


/************************/
/* function definitions */
/************************/
int lt_init(struct lt_configuration_type config)
{
  struct reflector_model_type rm;
  filterfactor = config.filterfactor;
  angle_scalefactor = config.angle_scalefactor;

  ccb.device.category = tir4_camera;
  ccb.mode = operational_3dot;
  if(cal_init(&ccb)!= 0){
    return -1;
  }
  cal_set_good_indication(&ccb, true);
  cal_thread_start(&ccb);

  rm.p1[0] = -35.0;
  rm.p1[1] = -50.0;
  rm.p1[2] = -92.5;
  rm.p2[0] = +35.0;
  rm.p2[1] = -50.0;
  rm.p2[2] = -92.5;
  rm.hc[0] = +0.0;
  rm.hc[1] = -100.0;
  rm.hc[2] = +90.0;
  pose_init(rm, 0.0);

  filtered_bloblist.num_blobs = 3;
  filtered_bloblist.blobs = filtered_blobs;
  first_frame_read = false;
  return 0;
}


int lt_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz)
{
  float raw_heading = 0.0;
  float raw_pitch = 0.0;
  float raw_roll = 0.0;
  float raw_tx = 0.0;
  float raw_ty = 0.0;
  float raw_tz = 0.0;
  static float filtered_heading = 0;
  static float filtered_pitch = 0;
  static float filtered_roll = 0;
  static float filtered_tx = 0;
  static float filtered_ty = 0;
  static float filtered_tz = 0;
  struct transform t;
  struct frame_type frame;
  bool frame_valid;
  cal_thread_get_frame(&frame, 
                       &frame_valid);
  if (frame_valid) {
    pose_sort_blobs(frame.bloblist);
    int i;
    for(i=0;i<3;i++) {
      if (first_frame_read) {
        filtered_bloblist.blobs[i].x = expfilt(frame.bloblist.blobs[i].x,
                                               filtered_bloblist.blobs[i].x,
                                               filterfactor);
        filtered_bloblist.blobs[i].y = expfilt(frame.bloblist.blobs[i].y,
                                               filtered_bloblist.blobs[i].y,
                                               filterfactor);
      }
      else {
        first_frame_read = true;
        filtered_bloblist.blobs[i].x = frame.bloblist.blobs[i].x;
        filtered_bloblist.blobs[i].y = frame.bloblist.blobs[i].y;
      }
    }
/*     printf("*** RAW blobs ***\n"); */
/*     bloblist_print(frame.bloblist); */
/*     printf("*** filtered blobs ***\n"); */
/*     bloblist_print(filtered_bloblist); */

    pose_process_blobs(filtered_bloblist, &t);
    pose_compute_camera_update(t,
                               &raw_heading,
                               &raw_pitch,
                               &raw_roll,
                               &raw_tx,
                               &raw_ty,
                               &raw_tz);
    frame_free(&ccb, &frame);
    if ((raw_heading < 180.0) && (raw_heading > -180.0)) {
      filtered_heading = expfilt(raw_heading,
                                 filtered_heading,
                                 filterfactor);
    }
    if ((raw_pitch < 180.0) && (raw_pitch > -180.0)) {
      filtered_pitch = expfilt(raw_pitch,
                               filtered_pitch,
                               filterfactor);
    }
    if ((raw_roll < 180.0) && (raw_roll > -180.0)) {
      filtered_roll = expfilt(raw_roll,
                              filtered_roll,
                              filterfactor);
    }
    filtered_tx = expfilt(raw_tx,
                          filtered_tx,
                          filterfactor);
    filtered_ty = expfilt(raw_ty,
                          filtered_ty,
                          filterfactor);
    filtered_tz = expfilt(raw_tz,
                          filtered_tz,
                          filterfactor);
  }  
  *heading = angle_scalefactor * filtered_heading;
  *pitch = angle_scalefactor * filtered_pitch;
  *roll = angle_scalefactor * filtered_roll;
  *tx = filtered_tx;
  *ty = filtered_ty;
  *tz = filtered_tz;
}

int lt_shutdown(void)
{
  cal_thread_stop();
  cal_shutdown(&ccb);
}

void lt_recenter(void)
{
  pose_recenter();
}

float expfilt(float x, 
              float y_minus_1,
              float filterfactor) 
{
  float y;
  
  y = y_minus_1*(1.0-filterfactor) + filterfactor*x;

  return y;
}
