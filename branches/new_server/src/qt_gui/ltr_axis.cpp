#include "ltr_axis.h"
#include <assert.h>
#include <ltr_gui_prefs.h>
#include <pref_global.h>
#include "ltr_profiles.h"
#include <iostream>
LtrAxis::LtrAxis(const AppProfile* tmp_parent, enum axis_t a) : 
  parent(tmp_parent), axis(a)
{
  emit axisChanged(RELOAD);
}

LtrAxis::~LtrAxis()
{
}

void LtrAxis::reload()
{
  emit axisChanged(RELOAD);
}

bool LtrAxis::changeEnabled(bool enabled)
{
  ltr_int_set_axis_bool_param(axis, AXIS_ENABLED, enabled);
  emit axisChanged(ENABLED);
  return true; 
}

bool LtrAxis::changeLFactor(float val)
{
  bool res = ltr_int_set_axis_param(axis, AXIS_LMULT, val);
  if(res){
    emit axisChanged(LFACTOR);
  }
  return res; 
}

bool LtrAxis::changeRFactor(float val)
{
  bool res = ltr_int_set_axis_param(axis, AXIS_RMULT, val);
  if(res){
    emit axisChanged(RFACTOR);
  }
  return res; 
}

bool LtrAxis::changeLCurv(float val)
{
  bool res = ltr_int_set_axis_param(axis, AXIS_LCURV, val);
  if(res){
    emit axisChanged(LCURV);
  }
  return res; 
}

bool LtrAxis::changeRCurv(float val)
{
  bool res = ltr_int_set_axis_param(axis, AXIS_RCURV, val);
  if(res){
    emit axisChanged(RCURV);
  }
  return res; 
}

bool LtrAxis::changeDZone(float val)
{
  bool res = ltr_int_set_axis_param(axis, AXIS_DEADZONE, val);
  if(res){
    emit axisChanged(DZONE);
  }
  return res; 
}

bool LtrAxis::changeLLimit(float val)
{
  bool res = ltr_int_set_axis_param(axis, AXIS_LLIMIT, val);
  if(res){
    emit axisChanged(LLIMIT);
  }
  return res; 
}

bool LtrAxis::changeRLimit(float val)
{
  bool res = ltr_int_set_axis_param(axis, AXIS_RLIMIT, val);
  if(res){
    emit axisChanged(RLIMIT);
  }
  return res; 
}

bool LtrAxis::getEnabled()
{
  return ltr_int_get_axis_bool_param(axis, AXIS_ENABLED);
}

float LtrAxis::getLFactor()
{
  return ltr_int_get_axis_param(axis, AXIS_LMULT);
}

float LtrAxis::getRFactor()
{
  return ltr_int_get_axis_param(axis, AXIS_RMULT);
}

float LtrAxis::getLCurv()
{
  return ltr_int_get_axis_param(axis, AXIS_LCURV);
}

float LtrAxis::getRCurv()
{
  return ltr_int_get_axis_param(axis, AXIS_RCURV);
}

float LtrAxis::getDZone()
{
  return ltr_int_get_axis_param(axis, AXIS_DEADZONE);
}

float LtrAxis::getLLimit()
{
  return ltr_int_get_axis_param(axis, AXIS_LLIMIT);
}

float LtrAxis::getRLimit()
{
  return ltr_int_get_axis_param(axis, AXIS_RLIMIT);
}

float LtrAxis::getValue(float val)
{
  return ltr_int_val_on_axis(axis, val);
}

bool LtrAxis::isSymetrical()
{
  return ltr_int_is_symetrical(axis);
}
