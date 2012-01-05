#ifndef PREF_GLOBAL__H
#define PREF_GLOBAL__H

#include <stdbool.h>
#include "cal.h"
#include "pose.h"
#include "tracking.h"
#include "axis.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *ltr_int_get_device_section();
bool ltr_int_is_model_active();
bool ltr_int_get_device(struct camera_control_block *ccb);
bool ltr_int_get_model_setup(reflector_model_type *rm);
void ltr_int_announce_model_change();
bool ltr_int_model_changed(bool reset_flag);
bool ltr_int_get_filter_factor(float *ff);
int ltr_int_get_orientation();
void ltr_int_close_prefs();

#ifdef __cplusplus
}
#endif

#endif
