#ifndef TRACKING__H
#define TRACKING__H

#include <stdbool.h>
#include "cal.h"
#include "ltlib.h"
#include "axis.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Coding:
            bit0 (lsb) - invert camera X values
            bit1       - invert camera Y values
            bit2       - switch X and Y values (applied first!)
            bit4       - invert pitch, roll, X and Z translations (for tracking from behind)
*/
enum ORIENTATION {ORIENT_NOP = 0, ORIENT_FLIP_X = 1, ORIENT_FLIP_Y = 2, 
                   ORIENT_XCHG_XY = 4, ORIENT_FROM_BEHIND = 8};


extern pose_t ltr_int_orig_pose;

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
bool ltr_int_postprocess_axes(ltr_axes_t axes, pose_t *pose, pose_t *unfiltered);
/*
double ltr_int_nonlinfilt(double x, 
              double y_minus_1,
              double filtfactor);

void ltr_int_nonlinfilt_vec(double x[3], 
              double y_minus_1[3],
              double filtfactor[3],
              double res[3]);
*/
#ifdef __cplusplus
}
#endif


#endif
