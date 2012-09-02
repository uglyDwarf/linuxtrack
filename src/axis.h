#ifndef AXIS__H
#define AXIS__H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

struct ltr_axes;
typedef struct ltr_axes *ltr_axes_t;
#define LTR_AXES_T_INITIALIZER NULL

enum axis_t {PITCH, ROLL, YAW, TX, TY, TZ};
enum axis_param_t {AXIS_ENABLED, AXIS_DEADZONE,
                   AXIS_LCURV, AXIS_RCURV,
                   AXIS_MULT,
                   AXIS_LLIMIT, AXIS_RLIMIT,
                   AXIS_FILTER,
                   AXIS_FULL, AXIS_DEFAULT = 1024};

void ltr_int_init_axes(ltr_axes_t *axes, const char *sec_name);
void ltr_int_close_axes(ltr_axes_t *axes);
float ltr_int_val_on_axis(ltr_axes_t axes, enum axis_t id, float x);
float ltr_int_filter_axis(ltr_axes_t axes, enum axis_t id, float x, float *y_minus_1);

bool ltr_int_is_symetrical(ltr_axes_t axes, enum axis_t id);

bool ltr_int_set_axis_param(ltr_axes_t axes, enum axis_t id, enum axis_param_t param, float val);
float ltr_int_get_axis_param(ltr_axes_t axes, enum axis_t id, enum axis_param_t param);

bool ltr_int_set_axis_bool_param(ltr_axes_t axes, enum axis_t id, enum axis_param_t param, bool val);
bool ltr_int_get_axis_bool_param(ltr_axes_t axes, enum axis_t id, enum axis_param_t param);

bool ltr_int_axes_changed(ltr_axes_t axes, bool reset_flag);
bool ltr_int_get_axes_ff(ltr_axes_t axes, double ffs[]);

const char *ltr_int_axis_get_desc(enum axis_t id);
const char *ltr_int_axis_param_get_desc(enum axis_param_t id);

void ltr_int_axes_from_default(ltr_axes_t *axes);

#ifdef __cplusplus
}
#endif

#endif
