#ifndef TRACKING__H
#define TRACKING__H

#include <pthread.h>
#include <stdbool.h>
#include "cal.h"
#include "axis.h"

#ifdef __cplusplus
extern "C" {
#endif

struct lt_axes {
  struct axis_def *pitch_axis;
  struct axis_def *yaw_axis;
  struct axis_def *roll_axis;
  struct axis_def *tx_axis;
  struct axis_def *ty_axis;
  struct axis_def *tz_axis;
};

struct current_pose{
  float heading;
  float pitch;
  float roll;
  float tx;
  float ty;
  float tz;
};

extern struct current_pose lt_orig_pose;

bool init_tracking();
int update_pose(struct frame_type *frame);
int recenter_tracking();
int get_camera_update(float *heading,
                      float *pitch,
                      float *roll,
                      float *tx,
                      float *ty,
                      float *tz);

#ifdef __cplusplus
}
#endif


#endif
