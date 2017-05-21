#ifndef PS3_PREFS__H
#define PS3_PREFS__H

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

#include "wc_driver_prefs.h"

typedef enum{
  e_HUE = 0,
  e_SATURATION,
  e_AUTOGAIN,
  e_AUTOWHITEBALANCE,
  e_AUTOEXPOSURE,
  e_GAIN,
  e_EXPOSURE,
  e_BRIGHTNESS,
  e_CONTRAST,
  e_SHARPNESS,
  e_HFLIP,
  e_VFLIP,
  e_PLFREQ,
  e_FPS,
  e_NUMCTRLS
} t_controls;

bool ltr_int_ps3_init_prefs(void);
bool ltr_int_ps3_close_prefs();

bool ltr_int_ps3_set_ctrl_val(t_controls ctrl, int val);
int ltr_int_ps3_get_ctrl_val(t_controls ctrl);
int ltr_int_ps3_controls_changed(void);
bool ltr_int_ps3_ctrl_changed(t_controls ctrl);

//bool ltr_int_ps3_set_resolution(int w, int h);
bool ltr_int_ps3_get_resolution(int *w, int *h);

int ltr_int_ps3_get_mode(void);
bool ltr_int_ps3_set_mode(int val);

#ifdef __cplusplus
}
#endif

#endif
