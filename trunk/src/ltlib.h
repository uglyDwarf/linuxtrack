#ifndef LINUX_TRACK__H
#define LINUX_TRACK__H

#include "pose.h"
#include "pref.h"

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
int lt_suspend(void);
int lt_wakeup(void);
void lt_recenter(void);
int lt_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz);

bool lt_open_pref(char *key, pref_id *prf);
float lt_get_flt(pref_id prf);
int lt_get_int(pref_id prf);
char *lt_get_str(pref_id prf);
bool lt_set_flt(pref_id *prf, float f);
bool lt_set_int(pref_id *prf, int i);
bool lt_set_str(pref_id *prf, char *str);
bool lt_save_prefs();
bool lt_close_pref(pref_id *prf);


#endif

