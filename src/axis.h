#ifndef AXIS__H
#define AXIS__H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

enum axis_t {PITCH, ROLL, YAW, TX, TY, TZ};

void ltr_int_init_axes();
void ltr_int_close_axes();
float ltr_int_val_on_axis(enum axis_t id, float x);
bool ltr_int_is_symetrical(enum axis_t id);
void ltr_int_enable_axis(enum axis_t id);
void ltr_int_disable_axis(enum axis_t id);
bool ltr_int_is_enabled(enum axis_t id);
bool ltr_int_set_deadzone(enum axis_t id, float dz);
float ltr_int_get_deadzone(enum axis_t id);
bool ltr_int_set_lcurv(enum axis_t id, float c);
float ltr_int_get_lcurv(enum axis_t id);
bool ltr_int_set_rcurv(enum axis_t id, float c);
float ltr_int_get_rcurv(enum axis_t id);
bool ltr_int_set_lmult(enum axis_t id, float m1);
float ltr_int_get_lmult(enum axis_t id);
bool ltr_int_set_rmult(enum axis_t id, float m1);
float ltr_int_get_rmult(enum axis_t id);
bool ltr_int_set_limits(enum axis_t id, float lim);
float ltr_int_get_limits(enum axis_t id);
bool ltr_int_axes_changed(bool reset_flag);

#ifdef __cplusplus
}
#endif

#endif
