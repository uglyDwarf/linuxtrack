#ifndef TRACKING__H
#define TRACKING__H

#include <pthread.h>
#include <stdbool.h>
#include "cal.h"

#ifdef __cplusplus
extern "C" {
#endif

struct lt_scalefactors {
  float pitch_sf;
  float yaw_sf;
  float roll_sf;
  float tx_sf;
  float ty_sf;
  float tz_sf;
};

struct current_pose{
  float heading;
  float pitch;
  float roll;
  float tx;
  float ty;
  float tz;
};

extern struct current_pose lt_current_pose;
extern pthread_mutex_t pose_mutex;

bool init_tracking();
int update_pose(struct frame_type *frame);
int recenter_tracking();

#ifdef __cplusplus
}
#endif


#endif
