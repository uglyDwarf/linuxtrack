#include "ltr_axis.h"
#include <assert.h>
#include <ltr_gui_prefs.h>
#include <pref_global.h>
#include "ltr_profiles.h"
#include <iostream>
LtrAxis::LtrAxis(const AppProfile* tmp_parent ,const QString &p) : 
  parent(tmp_parent), prefix(p)
{
  get_axis(prefix.toAscii().data(), &axis, NULL);
  emit axisChanged(RELOAD);
}

LtrAxis::~LtrAxis()
{
  close_axis(&axis);
}

void LtrAxis::reload()
{
  close_axis(&axis);
  get_axis(prefix.toAscii().data(), &axis, NULL);
  emit axisChanged(RELOAD);
}

bool LtrAxis::changeEnabled(bool enabled)
{
  QString val;
  if(enabled){
    enable_axis(axis);
    val = "yes";
  }else{
    disable_axis(axis);
    val = "no";
  }
  PREF.setKeyVal(parent->getProfileName(), prefix + "-enabled", val);
  emit axisChanged(ENABLED);
  return true; 
}

bool LtrAxis::changeLFactor(float val)
{
  bool res = set_lmult(axis, val);
  if(res){
    PREF.setKeyVal(parent->getProfileName(), prefix + "-left-multiplier", val);
    emit axisChanged(LFACTOR);
  }
  return res; 
}

bool LtrAxis::changeRFactor(float val)
{
  bool res = set_rmult(axis, val);
  if(res){
    PREF.setKeyVal(parent->getProfileName(), prefix + "-right-multiplier", val);
    emit axisChanged(RFACTOR);
  }
  return res; 
}

bool LtrAxis::changeLCurv(float val)
{
  bool res = set_lcurv(axis, val);
  if(res){
    PREF.setKeyVal(parent->getProfileName(), prefix + "-left-curvature", val);
    emit axisChanged(LCURV);
  }
  return res; 
}

bool LtrAxis::changeRCurv(float val)
{
  bool res = set_rcurv(axis, val);
  if(res){
    PREF.setKeyVal(parent->getProfileName(), prefix + "-right-curvature", val);
    emit axisChanged(RCURV);
  }
  return res; 
}

bool LtrAxis::changeDZone(float val)
{
  bool res = set_deadzone(axis, val);
  if(res){
    PREF.setKeyVal(parent->getProfileName(), prefix + "-deadzone", val);
    emit axisChanged(DZONE);
  }
  return res; 
}

bool LtrAxis::changeLimits(float val)
{
  bool res = set_limits(axis, val);
  if(res){
    PREF.setKeyVal(parent->getProfileName(), prefix + "-limits", val);
    emit axisChanged(LIMITS);
  }
  return res; 
}

bool LtrAxis::getEnabled()
{
  return is_enabled(axis);
}

float LtrAxis::getLFactor()
{
  return get_lmult(axis);
}

float LtrAxis::getRFactor()
{
  return get_rmult(axis);
}

float LtrAxis::getLCurv()
{
  return get_lcurv(axis);
}

float LtrAxis::getRCurv()
{
  return get_rcurv(axis);
}

float LtrAxis::getDZone()
{
  return get_deadzone(axis);
}

float LtrAxis::getLimits()
{
  return get_limits(axis);
}

float LtrAxis::getValue(float val)
{
  return val_on_axis(axis, val);
}
