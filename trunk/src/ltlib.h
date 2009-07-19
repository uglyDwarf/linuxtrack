#ifndef LINUX_TRACK__H
#define LINUX_TRACK__H

#include "pose.h"

struct lt_configuration_type {
  struct cal_device_type device;  
};

int lt_init(struct lt_configuration_type config);
int lt_get_transform(struct transform *t);
int lt_shutdown(void);
void lt_center(void);
int lt_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz);

#endif

