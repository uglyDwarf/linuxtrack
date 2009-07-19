#include "ltlib.h"

static struct camera_control_block ccb;

int lt_init(struct lt_configuration_type config)
{
  struct reflector_model_type rm;
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

  return 0;
}


int lt_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz)
{
  static float last_heading = 0.0;
  static float last_pitch = 0.0;
  static float last_roll = 0.0;
  static float last_tx = 0.0;
  static float last_ty = 0.0;
  static float last_tz = 0.0;

  struct transform t;
  struct frame_type frame;
  bool frame_valid;
  cal_thread_get_frame(&frame, 
                       &frame_valid);
  if (frame_valid) {
    pose_process_blobs(frame.bloblist, &t);
    pose_compute_camera_update(t,
                               heading,
                               pitch,
                               roll,
                               tx,
                               ty,
                               tz);
    frame_free(&ccb, &frame);
    last_heading = *heading;
    last_pitch = *pitch;
    last_roll = *roll;
    last_tx = *tx;
    last_ty = *ty;
    last_tz = *tz;
  }  
  else {
    *heading = last_heading;
    *pitch = last_pitch;
    *roll = last_roll;
    *tx = last_tx;
    *ty = last_ty;
    *tz = last_tz;
  }
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

