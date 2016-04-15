#include <stdlib.h>
#include "joy_driver_prefs.h"
#include "pref.h"
#include "pref_global.h"
#include "utils.h"

static const char *pitchKey = "Pitch";
static const char *yawKey = "Yaw";
static const char *rollKey = "Roll";
static const char *txKey = "Tx";
static const char *tyKey = "Ty";
static const char *tzKey = "Tz";
static const char *angleKey = "Angle";
static const char *transKey = "Trans";
static const char *ifaceKey = "Interface";
static const char *ppsKey = "PollsPerSecond";
static const char *jsVal = "Js";
static const char *evdevVal = "Evdev";

static int pitchAxis, yawAxis, rollAxis, txAxis, tyAxis, tzAxis;
static float angleBase = 16.0; //
static float translationBase = 60.0;
static int pollsPerSecond = 60;
static ifc_type_t interface = e_JS;

bool ltr_int_joy_init_prefs()
{
  const char *dev = ltr_int_get_device_section();
  if(dev == NULL){
    return false;
  }
  if(!ltr_int_get_key_int(dev, pitchKey, &pitchAxis)){
    pitchAxis = -1;
  }
  if(!ltr_int_get_key_int(dev, yawKey, &yawAxis)){
    yawAxis = -1;
  }
  if(!ltr_int_get_key_int(dev, rollKey, &rollAxis)){
    rollAxis = -1;
  }
  if(!ltr_int_get_key_int(dev, txKey, &txAxis)){
    txAxis = -1;
  }
  if(!ltr_int_get_key_int(dev, tyKey, &tyAxis)){
    tyAxis = -1;
  }
  if(!ltr_int_get_key_int(dev, tzKey, &tzAxis)){
    tzAxis = -1;
  }
  if(!ltr_int_get_key_flt(dev, angleKey, &angleBase)){
    angleBase = 16.0;
  }
  if(!ltr_int_get_key_flt(dev, transKey, &translationBase)){
    translationBase = 60.0;
  }
  if(!ltr_int_get_key_int(dev, ppsKey, &pollsPerSecond)){
    pollsPerSecond = 60;
  }
  interface = e_JS;
  char *tmp = ltr_int_get_key(dev, ifaceKey);
  if((tmp != NULL) && (strcmp(tmp, evdevVal) == 0)){
    interface = e_EVDEV;
  }
  if(tmp){
    free(tmp);
  }

  ltr_int_log_message("PitchAxis = %d\n", pitchAxis);
  ltr_int_log_message("YawAxis = %d\n", yawAxis);
  ltr_int_log_message("RollAxis = %d\n", rollAxis);
  ltr_int_log_message("TxAxis = %d\n", txAxis);
  ltr_int_log_message("TyAxis = %d\n", tyAxis);
  ltr_int_log_message("TzAxis = %d\n", tzAxis);
  ltr_int_log_message("AngleBase = %g\n", angleBase);
  ltr_int_log_message("TransBase = %g\n", translationBase);
  ltr_int_log_message("PollsPerSecond = %d\n", pollsPerSecond);
  ltr_int_log_message("Interface = %s\n", (interface == e_EVDEV)? "Evdev" : "Js");

  return true;
}


int ltr_int_joy_get_pitch_axis()
{
  return pitchAxis;
}

bool ltr_int_joy_set_pitch_axis(int val)
{
  pitchAxis = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), pitchKey, val);
}

int ltr_int_joy_get_yaw_axis()
{
  return yawAxis;
}

bool ltr_int_joy_set_yaw_axis(int val)
{
  yawAxis = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), yawKey, val);
}

int ltr_int_joy_get_roll_axis()
{
  return rollAxis;
}

bool ltr_int_joy_set_roll_axis(int val)
{
  rollAxis = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), rollKey, val);
}

int ltr_int_joy_get_tx_axis()
{
  return txAxis;
}

bool ltr_int_joy_set_tx_axis(int val)
{
  txAxis = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), txKey, val);
}

int ltr_int_joy_get_ty_axis()
{
  return tyAxis;
}

bool ltr_int_joy_set_ty_axis(int val)
{
  tyAxis = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), tyKey, val);
}

int ltr_int_joy_get_tz_axis()
{
  return tzAxis;
}

bool ltr_int_joy_set_tz_axis(int val)
{
  tzAxis = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), tzKey, val);
}

ifc_type_t ltr_int_joy_get_ifc()
{
  return interface;
}

bool ltr_int_joy_set_ifc(ifc_type_t val)
{
  interface = val;
  return ltr_int_change_key(ltr_int_get_device_section(), ifaceKey, (val == e_JS) ? jsVal : evdevVal);
}

float ltr_int_joy_get_angle_base()
{
  return angleBase;
}

bool ltr_int_joy_set_angle_base(float val)
{
  angleBase = val;
  return ltr_int_change_key_flt(ltr_int_get_device_section(), angleKey, val);
}

float ltr_int_joy_get_trans_base()
{
  return translationBase;
}

bool ltr_int_joy_set_trans_base(float val)
{
  translationBase = val;
  return ltr_int_change_key_flt(ltr_int_get_device_section(), transKey, val);
}

int ltr_int_joy_get_pps()
{
  return pollsPerSecond;
}

bool ltr_int_joy_set_pps(int val)
{
  pollsPerSecond = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), ppsKey, val);
}
