#ifndef PREF_GLOBAL__H
#define PREF_GLOBAL__H

#include <stdbool.h>
#include "cal.h"
#include "pose.h"

bool get_device(cal_device_category_type *dev_type_enum);
bool get_pose_setup(reflector_model_type *rm);

#endif
