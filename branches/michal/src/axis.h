#ifndef AXIS__H
#define AXIS__H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

struct axis_def;

void init_axis(struct axis_def **axis);
void close_axis(struct axis_def **axis);
float val_on_axis(struct axis_def *axis, float x);
bool is_symetrical(const struct axis_def *axis);
bool set_deadzone(struct axis_def *axis, float dz);
float get_deadzone(struct axis_def *axis);
bool set_lcurv(struct axis_def *axis, float c);
float get_lcurv(struct axis_def *axis);
bool set_rcurv(struct axis_def *axis, float c);
float get_rcurv(struct axis_def *axis);
bool set_lmult(struct axis_def *axis, float m1);
float get_lmult(struct axis_def *axis);
bool set_rmult(struct axis_def *axis, float m1);
float get_rmult(struct axis_def *axis);
bool set_limits(struct axis_def *axis, float lim);
float get_limits(struct axis_def *axis);

#ifdef __cplusplus
}
#endif

#endif
