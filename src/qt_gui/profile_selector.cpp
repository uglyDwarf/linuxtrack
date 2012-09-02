
#include "profile_selector.h"
#include "profile_setup.h"
#include "ltr_profiles.h"
#include "ltr_gui_prefs.h"
#include "utils.h"
#include "tracker.h"

ProfileSelector::ProfileSelector(QWidget *parent) : QWidget(parent), ps(NULL), initializing(true)
{
  ui.setupUi(this);
  
  //To make sure that at least default exists
  TRACKER.setProfile("Default");
  
  QStringList profiles;
  ui.Profiles->addItems(Profile::getProfiles().getProfileNames());
  initializing = false;
}


ProfileSelector::~ProfileSelector()
{
  if(ps != NULL){
    ui.AxesSetup->removeWidget(ps);
    delete ps;
    ps = NULL;
  }
}

void ProfileSelector::refresh()
{
  
}

void ProfileSelector::on_Profiles_currentIndexChanged(const QString &text)
{
  if(ps != NULL){
    ui.AxesSetup->removeWidget(ps);
    delete ps;
    ps = NULL;
  }
  ps = new ProfileSetup(text, this);
  ui.AxesSetup->addWidget(ps);
}


