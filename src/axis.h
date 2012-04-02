#ifndef AXIS__H
#define AXIS__H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

enum axis_t {PITCH, ROLL, YAW, TX, TY, TZ};
enum axis_param_t {AXIS_ENABLED, AXIS_DEADZONE, 
                   AXIS_LCURV, AXIS_RCURV, 
                   AXIS_LMULT, AXIS_RMULT,
                   AXIS_LLIMIT, AXIS_RLIMIT};

void ltr_int_init_axes();
void ltr_int_close_axes();
float ltr_int_val_on_axis(enum axis_t id, float x);

bool ltr_int_is_symetrical(enum axis_t id);

bool ltr_int_set_axis_param(enum axis_t id, enum axis_param_t param, float val);
float ltr_int_get_axis_param(enum axis_t id, enum axis_param_t param);

bool ltr_int_set_axis_bool_param(enum axis_t id, enum axis_param_t param, bool val);
bool ltr_int_get_axis_bool_param(enum axis_t id, enum axis_param_t param);

bool ltr_int_axes_changed(bool reset_flag);

#ifdef __cplusplus
}
#endif

#endif
