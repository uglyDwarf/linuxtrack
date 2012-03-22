#ifndef TRACKING__H
#define TRACKING__H

#include <pthread.h>
#include <stdbool.h>
#include "cal.h"

#ifdef __cplusplus
extern "C" {
#endif

struct current_pose{
  double heading;
  double pitch;
  double roll;
  double tx;
  double ty;
  double tz;
};

extern struct current_pose ltr_int_orig_pose;

bool ltr_int_init_tracking();
int ltr_int_update_pose(struct frame_type *frame);
int ltr_int_recenter_tracking();
int ltr_int_tracking_get_camera(float *heading,
                      float *pitch,
                      float *roll,
                      float *tx,
                      float *ty,
                      float *tz,
                      unsigned int *counter);

double ltr_int_nonlinfilt(double x, 
              double y_minus_1,
              double filtfactor);

void ltr_int_nonlinfilt_vec(double x[3], 
              double y_minus_1[3],
              double filtfactor[3],
              double res[3]);

#ifdef __cplusplus
}
#endif


#endif
