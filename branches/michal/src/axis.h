#ifndef AXIS__H
#define AXIS__H

#ifdef __cplusplus
extern "C" {
#endif

#include "spline.h"
#include <stdbool.h>

typedef struct{
  splines_def curve_defs;
  splines curves;
  bool valid;
  float l_factor, r_factor;
  float limits; //Input value limits - normalize into <-1;1>
} axis_def;

float val_on_axis(axis_def *axis, float x);
bool is_symetrical(const axis_def *axis);;
bool set_deadzone(axis_def *axis, float dz);
bool set_lcurv(axis_def *axis, float c);
bool set_rcurv(axis_def *axis, float c);
bool set_lmult(axis_def *axis, float m1);
bool set_rmult(axis_def *axis, float m1);
bool set_limits(axis_def *axis, float lim);

#ifdef __cplusplus
}
#endif

#endif
