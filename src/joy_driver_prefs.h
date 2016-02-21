#ifndef JOY_DRIVER_PREFS__H
#define JOY_DRIVER_PREFS__H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  e_JS,
  e_EVDEV
} ifc_type_t;


typedef struct {
  char **nameList;
  size_t nameListSize;
  size_t namesFound;
}joystickNames_t;

typedef struct {
  uint8_t *axesList;
  const char **axisNames;
  int *min;
  int *max;
  size_t axes;
} axes_t;

bool ltr_int_joy_init_prefs();
int ltr_int_joy_get_pitch_axis();
bool ltr_int_joy_set_pitch_axis(int val);
int ltr_int_joy_get_yaw_axis();
bool ltr_int_joy_set_yaw_axis(int val);
int ltr_int_joy_get_roll_axis();
bool ltr_int_joy_set_roll_axis(int val);
int ltr_int_joy_get_tx_axis();
bool ltr_int_joy_set_tx_axis(int val);
int ltr_int_joy_get_ty_axis();
bool ltr_int_joy_set_ty_axis(int val);
int ltr_int_joy_get_tz_axis();
bool ltr_int_joy_set_tz_axis(int val);
ifc_type_t ltr_int_joy_get_ifc();
bool ltr_int_joy_set_ifc(ifc_type_t val);
float ltr_int_joy_get_angle_base();
bool ltr_int_joy_set_angle_base(float val);
float ltr_int_joy_get_trans_base();
bool ltr_int_joy_set_trans_base(float val);
int ltr_int_joy_get_pps();
bool ltr_int_joy_set_pps(int val);

bool ltr_int_joy_enum_axes(ifc_type_t ifc, const char *name, axes_t *axes);
void ltr_int_joy_free_axes(axes_t axes);
joystickNames_t *ltr_int_joy_enum_joysticks(ifc_type_t ifc);
void ltr_int_joy_free_joysticks(joystickNames_t *nl);

#ifdef __cplusplus
}
#endif


#endif
