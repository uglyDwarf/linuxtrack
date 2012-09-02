#ifndef PROFILE_SETUP__H
#define PROFILE_SETUP__H

#include <QWidget>
#include "ui_profile_setup.h"

class ScpForm;

class ProfileSetup : public QWidget
{
 Q_OBJECT
 public:
  ProfileSetup(const QString &name, QWidget *parent = 0);
  ~ProfileSetup();
 private:
   void connect();
   Ui::ProfileSetupForm ui;
   ScpForm *sc;
   QString profileName;
   bool initializing;
 private slots:
   void on_DetailedAxisSetup_pressed();
   void on_CopyFromDefault_pressed();
   void on_PitchEnable_stateChanged(int state);
   void on_YawEnable_stateChanged(int state);
   void on_RollEnable_stateChanged(int state);
   void on_TxEnable_stateChanged(int state);
   void on_TyEnable_stateChanged(int state);
   void on_TzEnable_stateChanged(int state);
   void on_PitchSens_valueChanged(int val);
   void on_YawSens_valueChanged(int val);
   void on_RollSens_valueChanged(int val);
   void on_TxSens_valueChanged(int val);
   void on_TySens_valueChanged(int val);
   void on_TzSens_valueChanged(int val);
   void on_Smoothing_valueChanged(int val);
   void axisChanged(int axis, int elem);
   void setCommonFF(float val);
   void initAxes();
};

#endif
