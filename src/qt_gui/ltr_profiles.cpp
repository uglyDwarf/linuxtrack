#include <ltr_gui_prefs.h>
#include <ltr_profiles.h>
#include "tracker.h"
#include <iostream>

//TODO!!!: check the code, maybe move filterfactor to the TRACKER too...
//         check switching profiles!!!

Profile::Profile() : currentProfile(NULL)
{
  PREF.getProfiles(names);
  currentName = QString::fromUtf8("Default");
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
  ltr_axes_t tmp_axes = LTR_AXES_T_INITIALIZER;
  ltr_int_init_axes(&tmp_axes, newSec.toUtf8().constData());
  ltr_int_close_axes(&tmp_axes);
  PREF.getProfiles(names);
/*  
  QString section_name = "Profile";
  PREF.createSection(section_name);
  PREF.addKeyVal(section_name, "Title", newSec);
  names.append(newSec);
*/
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
  currentProfile->changeProfile(name);
  return true;
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
  return names.indexOf(name);
}


AppProfile::AppProfile(const QString &n, QWidget *parent) : QWidget(parent), name(n),
                                                            initializing(false)
{
  TRACKER.setProfile(name);
}

AppProfile::~AppProfile()
{
}

const QString &AppProfile::getProfileName() const
{
  return name;
}

bool AppProfile::changeProfile(const QString &newName)
{
  //std::cout<<"Approfile changing profile to "<<newName.toStdString()<<"\n";

  name = newName;
  TRACKER.setProfile(name);
  return true;
}

#include "moc_ltr_profiles.cpp"

