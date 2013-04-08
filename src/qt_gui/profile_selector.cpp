#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>

#include "profile_selector.h"
#include "profile_setup.h"
#include "ltr_profiles.h"
#include "ltr_gui_prefs.h"
#include "utils.h"
#include "tracker.h"
#include <iostream>

ProfileSelector::ProfileSelector(QWidget *parent) : QWidget(parent), ps(NULL), initializing(true)
{
  ui.setupUi(this);
  
  //To make sure that at least default exists
  TRACKER.setProfile("Default");
  
  QStringList profiles;
  ui.Profiles->addItems(Profile::getProfiles().getProfileNames());
  initializing = false;
  setCurrentProfile("Default");
//  on_Profiles_currentIndexChanged("Default");
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
  QString currentItem = ui.Profiles->currentText();
  QStringList profiles;
  initializing = true;
  ui.Profiles->clear();
  ui.Profiles->addItems(Profile::getProfiles().getProfileNames());
  initializing = false;
  setCurrentProfile(currentItem);
}

bool ProfileSelector::setCurrentProfile(QString prof)
{
  int index = ui.Profiles->findText(prof);
  if(index == -1){
    std::cout<<"Profile "<<prof.toStdString()<<" not found!!!"<<std::endl;
    return false;
  }
  std::cout<<"Profile "<<prof.toStdString()<<" found, setting it!!!"<<std::endl;
  ui.Profiles->setCurrentIndex(index);
  return true;
}

void ProfileSelector::on_Profiles_currentIndexChanged(const QString &text)
{
  if((PROFILE.isProfile(text)) < 0){
    return;
  }
  if(ps != NULL){
    ui.AxesSetup->removeWidget(ps);
    delete ps;
    ps = NULL;
  }
  std::cout<<"Changed index to "<<text.toStdString()<<std::endl;
  ps = new ProfileSetup(text, this);
  ui.AxesSetup->insertWidget(1, ps);
}

void ProfileSelector::on_CopyFromDefault_pressed()
{
  ps->copyFromDefault();
}

void ProfileSelector::on_ImportProfile_pressed()
{
  QString home = qgetenv("HOME").constData();
  QString mask("Profile (*.profile)");
  QString fname = QFileDialog::getOpenFileName(this, "Import Profile...", home, mask, &mask);
  if(fname == ""){
    return;
  }
  QFile f(fname);
  if(!f.open(QIODevice::ReadOnly)){
    QMessageBox::warning(this, "Problem importing profile!", 
      "There was a problem opening the profile file '" + fname + "'!" );
    return;
  }
  QTextStream tf(&f);

  QString newName(tf.readLine());
  std::cout<<"Importing profile '"<<newName.toStdString()<<"'"<<std::endl;

int ccc = PROFILE.isProfile(newName);
std::cout<<"..."<<ccc<<std::endl;
  if(ccc < 0){
    std::cout<<"Creating new profile!"<<std::endl;
    PROFILE.addProfile(newName);
    refresh();
  }
  setCurrentProfile(newName);
  ps->importProfile(tf);
}

void ProfileSelector::on_ExportProfile_pressed()
{
  QString home = qgetenv("HOME").constData();
  QString mask("Profile (*.profile)");
  QString fname = QFileDialog::getSaveFileName(this, "Export Profile...", home, mask, &mask);
  if(fname == ""){
    return;
  }
  if(!fname.endsWith(".profile")){
    fname += ".profile";
  }
  QFile f(fname);
  if(!f.open(QIODevice::WriteOnly | QFile::Truncate)){
    QMessageBox::warning(this, "Problem exporting profile!", 
      "There was a problem saving the profile to file '" + fname + "'!" );
    return;
  }
  QTextStream tf(&f);

  ps->exportProfile(tf);
}

