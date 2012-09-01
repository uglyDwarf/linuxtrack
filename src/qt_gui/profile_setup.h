#ifndef PROFILE_SETUP__H
#define PROFILE_SETUP__H

#include <QWidget>
#include "ui_profile_setup.h"

class ScpForm;

class ProfileSetup : public QWidget
{
 Q_OBJECT
 public:
  ProfileSetup(QWidget *parent = 0);
  ~ProfileSetup();
 private:
   Ui::ProfileSetupForm ui;
   ScpForm *sc;
 private slots:
   void on_DetailedAxisSetup_pressed();
};


#endif
