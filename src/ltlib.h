#ifndef LINUX_TRACK__H
#define LINUX_TRACK__H

#include "pose.h"

struct lt_configuration_type {
  struct cal_device_type device;  
  /* 1.0 for raw, the closer to zero, the more filtering */
//  float filterfactor;  
//  float angle_scalefactor;  
};

struct lt_scalefactors {
  float pitch_sf;
  float yaw_sf;
  float roll_sf;
  float tx_sf;
  float ty_sf;
  float tz_sf;
};

int lt_init(struct lt_configuration_type config, char *cust_section);
int lt_get_transform(struct transform *t);
int lt_shutdown(void);
void lt_recenter(void);
int lt_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz);

#endif

