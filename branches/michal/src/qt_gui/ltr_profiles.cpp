#include <ltr_gui_prefs.h>
#include <ltr_profiles.h>
#include <iostream>

Profile::Profile() : currentProfile(NULL)
{
  PREF.getProfiles(names);
  currentName = "Default";
  currentProfile = new AppProfile(currentName);
}

Profile *Profile::prof = NULL;

Profile& Profile::getProfiles()
{
  if(prof == NULL){
    prof = new Profile();
  }
  return *prof;
}

void Profile::addProfile(const QString &newSec)
{
  QString section_name = "Profile";
  PREF.createSection(section_name);
  PREF.addKeyVal(section_name, "Title", newSec);
  names.append(newSec);
}

const QStringList &Profile::getProfileNames()
{
  return names;
}

bool Profile::setCurrent(const QString &name)
{
  if(!names.contains(name, Qt::CaseInsensitive)){
    return false;
  }
  if(PREF.setCustomSection(name)){
    currentProfile->changeProfile(PREF.getCustomSectionName());
    return true;
  }
  return false;
}

AppProfile *Profile::getCurrentProfile()
{
  return currentProfile;
}

const QString &Profile::getCurrentProfileName()
{
  return currentName;
}

int Profile::isProfile(const QString &name)
{
  int i = -1;
  if(names.contains(name, Qt::CaseInsensitive)){
    for(i = 0; i < names.size(); ++i){
      if(names[i].compare(name, Qt::CaseInsensitive) == 0){
        break;
      }
    }
  }
  return i;
}

AppProfile::AppProfile(const QString &n, QWidget *parent) : QWidget(parent), name(n), 
                                                            initializing(false)
{
  PREF.setCustomSection(name);
  //std::cout<<"Cust section: "<<name.toAscii().data()<<std::endl;
  pitch = new LtrAxis(this, PITCH);
  roll = new LtrAxis(this, ROLL);
  yaw = new LtrAxis(this, YAW);
  tx = new LtrAxis(this, TX);
  ty = new LtrAxis(this, TY);
  tz = new LtrAxis(this, TZ);
  filterFactorReload();
}

void AppProfile::filterFactorReload()
{
  QString val;
  PREF.getKeyVal("Filter-factor", val);
  float filterFactor = val.toFloat();
  emit filterFactorChanged(filterFactor);
}

AppProfile::~AppProfile()
{
  delete(pitch);
  delete(roll);
  delete(yaw);
  delete(tx);
  delete(ty);
  delete(tz);
}

void AppProfile::on_pitchChange(AxisElem_t what)
{
  emit pitchChanged(what);
}

void AppProfile::on_rollChange(AxisElem_t what)
{
  emit rollChanged(what);
}

void AppProfile::on_yawChange(AxisElem_t what)
{
  emit yawChanged(what);
}

void AppProfile::on_txChange(AxisElem_t what)
{
  emit txChanged(what);
}

void AppProfile::on_tyChange(AxisElem_t what)
{
  emit tyChanged(what);
}

void AppProfile::on_tzChange(AxisElem_t what)
{
  emit tzChanged(what);
}

LtrAxis *AppProfile::getPitchAxis()
{
  return pitch;
}

LtrAxis *AppProfile::getRollAxis()
{
  return roll;
}

LtrAxis *AppProfile::getYawAxis()
{
  return yaw;
}

LtrAxis *AppProfile::getTxAxis()
{
  return tx;
}

LtrAxis *AppProfile::getTyAxis()
{
  return ty;
}

LtrAxis *AppProfile::getTzAxis()
{
  return tz;
}

const QString &AppProfile::getProfileName() const
{
  return name;
}

bool AppProfile::changeProfile(const QString &newName)
{
  name = newName;
  pitch->reload();
  roll->reload();
  yaw->reload();
  tx->reload();
  ty->reload();
  tz->reload();
  filterFactorReload();
  return true;
}

void AppProfile::setFilterFactor(float f)
{
  filterFactor = f;
  emit filterFactorChanged(f);
}

float AppProfile::getFilterFactor()
{
  return filterFactor;
}

