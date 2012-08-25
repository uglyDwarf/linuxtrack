#include <ltr_gui_prefs.h>
#include <ltr_profiles.h>
#include "tracker.h"
#include <iostream>

//TODO!!!: check the code, maybe move filterfactor to the TRACKER too...
//         check switching profiles!!!

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
  //std::cout<<"Approfile changing profile to "<<newName.toStdString()<<std::endl;

  name = newName;
  TRACKER.setProfile(name);
  return true;
}


