#ifndef PREF_GLOBAL__H
#define PREF_GLOBAL__H

#include <stdbool.h>
#include "cal.h"
#include "pose.h"
#include "ltlib.h"

char *get_device_section();
char *get_storage_path();
bool get_ir_on(bool def);
bool get_device(struct camera_control_block *ccb);
bool get_pose_setup(reflector_model_type *rm, bool *model_changed);
bool get_scale_factors(struct lt_scalefactors *sf);
bool get_filter_factor(float *ff);

#endif
