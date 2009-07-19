#include "ltlib.h"

static struct camera_control_block ccb;

int lt_init(struct lt_configuration_type config)
{
  struct reflector_model_type rm;
  ccb.device.category = tir4_camera;
  ccb.mode = operational_3dot;
  cal_init(&ccb);
  cal_set_good_indication(&ccb, true);

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
  struct transform t;
  struct frame_type frame;
  
  cal_get_frame(&ccb, &frame);
  pose_process_blobs(frame.bloblist, &t);
/*   frame_print(frame); */
  frame_free(&ccb, &frame);
/*   transform_print(t); */
  pose_compute_camera_update(t,
                             heading,
                             pitch,
                             roll,
                             tx,
                             ty,
                             tz);

  
}

int lt_shutdown(void)
{
  cal_shutdown(&ccb);
}

void lt_recenter(void)
{
  pose_recenter();
}

