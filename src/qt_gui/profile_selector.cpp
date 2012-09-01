
#include "profile_selector.h"
#include "profile_setup.h"
#include "ltr_profiles.h"

ProfileSelector::ProfileSelector(QWidget *parent) : QWidget(parent), ps(NULL), initializing(true)
{
  ui.setupUi(this);
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
  ps = new ProfileSetup(this);
  ui.AxesSetup->addWidget(ps);
}


