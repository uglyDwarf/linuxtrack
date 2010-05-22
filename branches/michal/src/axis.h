#ifndef AXIS__H
#define AXIS__H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

struct axis_def;

void ltr_int_init_axis(struct axis_def **axis);
void ltr_int_close_axis(struct axis_def **axis);
float ltr_int_val_on_axis(struct axis_def *axis, float x);
bool ltr_int_is_symetrical(const struct axis_def *axis);
void ltr_int_enable_axis(struct axis_def *axis);
void ltr_int_disable_axis(struct axis_def *axis);
bool ltr_int_is_enabled(struct axis_def *axis);
bool ltr_int_set_deadzone(struct axis_def *axis, float dz);
float ltr_int_get_deadzone(struct axis_def *axis);
bool ltr_int_set_lcurv(struct axis_def *axis, float c);
float ltr_int_get_lcurv(struct axis_def *axis);
bool ltr_int_set_rcurv(struct axis_def *axis, float c);
float ltr_int_get_rcurv(struct axis_def *axis);
bool ltr_int_set_lmult(struct axis_def *axis, float m1);
float ltr_int_get_lmult(struct axis_def *axis);
bool ltr_int_set_rmult(struct axis_def *axis, float m1);
float ltr_int_get_rmult(struct axis_def *axis);
bool ltr_int_set_limits(struct axis_def *axis, float lim);
float ltr_int_get_limits(struct axis_def *axis);

#ifdef __cplusplus
}
#endif

#endif
