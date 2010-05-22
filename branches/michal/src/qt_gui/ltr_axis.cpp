#include "ltr_axis.h"
#include <assert.h>
#include <ltr_gui_prefs.h>
#include <pref_global.h>
#include "ltr_profiles.h"
#include <iostream>
LtrAxis::LtrAxis(const AppProfile* tmp_parent ,const QString &p) : 
  parent(tmp_parent), prefix(p)
{
  ltr_int_get_axis(prefix.toAscii().data(), &axis, NULL);
  emit axisChanged(RELOAD);
}

LtrAxis::~LtrAxis()
{
  ltr_int_close_axis(&axis);
}

void LtrAxis::reload()
{
  ltr_int_close_axis(&axis);
  ltr_int_get_axis(prefix.toAscii().data(), &axis, NULL);
  emit axisChanged(RELOAD);
}

bool LtrAxis::changeEnabled(bool enabled)
{
  QString val;
  if(enabled){
    ltr_int_enable_axis(axis);
    val = "yes";
  }else{
    ltr_int_disable_axis(axis);
    val = "no";
  }
  PREF.setKeyVal(parent->getProfileName(), prefix + "-enabled", val);
  emit axisChanged(ENABLED);
  return true; 
}

bool LtrAxis::changeLFactor(float val)
{
  bool res = ltr_int_set_lmult(axis, val);
  if(res){
    PREF.setKeyVal(parent->getProfileName(), prefix + "-left-multiplier", val);
    emit axisChanged(LFACTOR);
  }
  return res; 
}

bool LtrAxis::changeRFactor(float val)
{
  bool res = ltr_int_set_rmult(axis, val);
  if(res){
    PREF.setKeyVal(parent->getProfileName(), prefix + "-right-multiplier", val);
    emit axisChanged(RFACTOR);
  }
  return res; 
}

bool LtrAxis::changeLCurv(float val)
{
  bool res = ltr_int_set_lcurv(axis, val);
  if(res){
    PREF.setKeyVal(parent->getProfileName(), prefix + "-left-curvature", val);
    emit axisChanged(LCURV);
  }
  return res; 
}

bool LtrAxis::changeRCurv(float val)
{
  bool res = ltr_int_set_rcurv(axis, val);
  if(res){
    PREF.setKeyVal(parent->getProfileName(), prefix + "-right-curvature", val);
    emit axisChanged(RCURV);
  }
  return res; 
}

bool LtrAxis::changeDZone(float val)
{
  bool res = ltr_int_set_deadzone(axis, val);
  if(res){
    PREF.setKeyVal(parent->getProfileName(), prefix + "-deadzone", val);
    emit axisChanged(DZONE);
  }
  return res; 
}

bool LtrAxis::changeLimits(float val)
{
  bool res = ltr_int_set_limits(axis, val);
  if(res){
    PREF.setKeyVal(parent->getProfileName(), prefix + "-limits", val);
    emit axisChanged(LIMITS);
  }
  return res; 
}

bool LtrAxis::getEnabled()
{
  return ltr_int_is_enabled(axis);
}

float LtrAxis::getLFactor()
{
  return ltr_int_get_lmult(axis);
}

float LtrAxis::getRFactor()
{
  return ltr_int_get_rmult(axis);
}

float LtrAxis::getLCurv()
{
  return ltr_int_get_lcurv(axis);
}

float LtrAxis::getRCurv()
{
  return ltr_int_get_rcurv(axis);
}

float LtrAxis::getDZone()
{
  return ltr_int_get_deadzone(axis);
}

float LtrAxis::getLimits()
{
  return ltr_int_get_limits(axis);
}

float LtrAxis::getValue(float val)
{
  return ltr_int_val_on_axis(axis, val);
}

bool LtrAxis::isSymetrical()
{
  return ltr_int_is_symetrical(axis);
}
