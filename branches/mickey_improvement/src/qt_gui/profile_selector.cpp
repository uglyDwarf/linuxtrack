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
  TRACKER.setProfile(QString::fromUtf8("Default"));
  
  QStringList profiles;
  ui.Profiles->addItems(Profile::getProfiles().getProfileNames());
  initializing = false;
  setCurrentProfile(QString::fromUtf8("Default"));
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
    //std::cout<<"Profile "<<prof.toStdString()<<" not found!!!"<<std::endl;
    return false;
  }
  //std::cout<<"Profile "<<prof.toStdString()<<" found, setting it!!!"<<std::endl;
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
  //std::cout<<"Changed index to "<<text.toStdString()<<std::endl;
  ps = new ProfileSetup(text, this);
  ui.AxesSetup->insertWidget(1, ps);
}

void ProfileSelector::on_CopyFromDefault_pressed()
{
  ps->copyFromDefault();
}

void ProfileSelector::on_ImportProfile_pressed()
{
  QString home = QString::fromUtf8(qgetenv("HOME").constData());
  QString mask(QString::fromUtf8("Profile (*.profile)"));
  QString fname = QFileDialog::getOpenFileName(this, QString::fromUtf8("Import Profile..."), home, mask, &mask);
  if(fname == QString::fromUtf8("")){
    return;
  }
  QFile f(fname);
  if(!f.open(QIODevice::ReadOnly)){
    QMessageBox::warning(this, QString::fromUtf8("Problem importing profile!"), 
      QString::fromUtf8("There was a problem opening the profile file '") + fname + QString::fromUtf8("'!") );
    return;
  }
  QTextStream tf(&f);

  QString newName(tf.readLine());
  //std::cout<<"Importing profile '"<<newName.toStdString()<<"'"<<std::endl;

int ccc = PROFILE.isProfile(newName);
  //std::cout<<"..."<<ccc<<std::endl;
  if(ccc < 0){
    //std::cout<<"Creating new profile!"<<std::endl;
    PROFILE.addProfile(newName);
    refresh();
  }
  setCurrentProfile(newName);
  ps->importProfile(tf);
}

void ProfileSelector::on_ExportProfile_pressed()
{
  QString home = QString::fromUtf8(qgetenv("HOME").constData());
  QString mask(QString::fromUtf8("Profile (*.profile)"));
  QString fname = QFileDialog::getSaveFileName(this, QString::fromUtf8("Export Profile..."), home, mask, &mask);
  if(fname == QString::fromUtf8("")){
    return;
  }
  if(!fname.endsWith(QString::fromUtf8(".profile"))){
    fname += QString::fromUtf8(".profile");
  }
  QFile f(fname);
  if(!f.open(QIODevice::WriteOnly | QFile::Truncate)){
    QMessageBox::warning(this, QString::fromUtf8("Problem exporting profile!"), 
      QString::fromUtf8("There was a problem saving the profile to file '") + fname + QString::fromUtf8("'!") );
    return;
  }
  QTextStream tf(&f);

  ps->exportProfile(tf);
}

bool ProfileSelector::close()
{
  ps->close();
  return QWidget::close();
}


