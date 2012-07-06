#include <ltr_tracking.h>
#include <ltr_profiles.h>
#include <ltr_gui_prefs.h>
#include <QInputDialog>
#include <iostream>
#include "tracker.h"

static bool state2bool(int state);
static Qt::CheckState bool2state(bool v);


LtrTracking::LtrTracking(const Ui::LinuxtrackMainForm &ui) : gui(ui), initializing(false)
{
  Connect();
  initializing= true;
  gui.Profiles->addItems(Profile::getProfiles().getProfileNames());
  initializing = false;
}

LtrTracking::~LtrTracking()
{
}

void LtrTracking::refresh()
{
  PROFILE.setCurrent("Default");
  emit customSectionChanged();
}

void LtrTracking::Connect()
{
  QObject::connect(&TRACKER, SIGNAL(axisChanged(int, int)), 
                    this, SLOT(axisChanged(int, int)));
  QObject::connect(gui.Profiles, SIGNAL(currentIndexChanged(const QString &)), 
                    this, SLOT(on_Profiles_currentIndexChanged(const QString &)));
  QObject::connect(gui.CreateNewProfile, SIGNAL(pressed()), 
                    this, SLOT(on_CreateNewProfile_pressed()));
  QObject::connect(gui.PitchEnable, SIGNAL(stateChanged(int)),
                    this, SLOT(on_PitchEnable_stateChanged(int)));
  QObject::connect(gui.RollEnable, SIGNAL(stateChanged(int)),
                    this, SLOT(on_RollEnable_stateChanged(int)));
  QObject::connect(gui.YawEnable, SIGNAL(stateChanged(int)),
                    this, SLOT(on_YawEnable_stateChanged(int)));
  QObject::connect(gui.XEnable, SIGNAL(stateChanged(int)),
                    this, SLOT(on_XEnable_stateChanged(int)));
  QObject::connect(gui.YEnable, SIGNAL(stateChanged(int)),
                    this, SLOT(on_YEnable_stateChanged(int)));
  QObject::connect(gui.ZEnable, SIGNAL(stateChanged(int)),
                    this, SLOT(on_ZEnable_stateChanged(int)));
  QObject::connect(gui.PitchUpSpin, SIGNAL(valueChanged(double)),
                    this, SLOT(on_PitchUpSpin_valueChanged(double)));
  QObject::connect(gui.PitchDownSpin, SIGNAL(valueChanged(double)),
                    this, SLOT(on_PitchDownSpin_valueChanged(double)));
  QObject::connect(gui.YawLeftSpin, SIGNAL(valueChanged(double)),
                    this, SLOT(on_YawLeftSpin_valueChanged(double)));
  QObject::connect(gui.YawRightSpin, SIGNAL(valueChanged(double)),
                    this, SLOT(on_YawRightSpin_valueChanged(double)));
  QObject::connect(gui.TiltLeftSpin, SIGNAL(valueChanged(double)),
                    this, SLOT(on_TiltLeftSpin_valueChanged(double)));
  QObject::connect(gui.TiltRightSpin, SIGNAL(valueChanged(double)),
                    this, SLOT(on_TiltRightSpin_valueChanged(double)));
  QObject::connect(gui.MoveLeftSpin, SIGNAL(valueChanged(double)),
                    this, SLOT(on_MoveLeftSpin_valueChanged(double)));
  QObject::connect(gui.MoveRightSpin, SIGNAL(valueChanged(double)),
                    this, SLOT(on_MoveRightSpin_valueChanged(double)));
  QObject::connect(gui.MoveUpSpin, SIGNAL(valueChanged(double)),
                    this, SLOT(on_MoveUpSpin_valueChanged(double)));
  QObject::connect(gui.MoveDownSpin, SIGNAL(valueChanged(double)),
                    this, SLOT(on_MoveDownSpin_valueChanged(double)));
  QObject::connect(gui.MoveBackSpin, SIGNAL(valueChanged(double)),
                    this, SLOT(on_MoveBackSpin_valueChanged(double)));
  QObject::connect(gui.MoveForthSpin, SIGNAL(valueChanged(double)),
                    this, SLOT(on_MoveForthSpin_valueChanged(double)));
  QObject::connect(&TRACKER, SIGNAL(setCommonFF(float)),
                    this, SLOT(setCommonFF(float)));
  QObject::connect(gui.CommonFF, SIGNAL(valueChanged(int)),
                    this, SLOT(on_CommonFF_valueChanged(int)));
  QObject::connect(&TRACKER, SIGNAL(initAxes(void)),
                    this, SLOT(initAxes(void)));
}

void LtrTracking::axisChanged(int axis, int elem)
{
  switch(axis){
    case PITCH:
      if((elem == AXIS_ENABLED) || (elem == AXIS_FULL)) gui.PitchEnable->setCheckState(bool2state(TRACKER.axisGetEnabled(PITCH)));
      if((elem == AXIS_LMULT) || (elem == AXIS_FULL)) gui.PitchUpSpin->setValue(TRACKER.axisGet(PITCH, AXIS_LMULT));
      if((elem == AXIS_RMULT) || (elem == AXIS_FULL)) gui.PitchDownSpin->setValue(TRACKER.axisGet(PITCH, AXIS_RMULT));
      break;
    case ROLL:
      if((elem == AXIS_ENABLED) || (elem == AXIS_FULL)) gui.RollEnable->setCheckState(bool2state(TRACKER.axisGetEnabled(ROLL)));
      if((elem == AXIS_LMULT) || (elem == AXIS_FULL)) gui.TiltLeftSpin->setValue(TRACKER.axisGet(ROLL, AXIS_LMULT));
      if((elem == AXIS_RMULT) || (elem == AXIS_FULL)) gui.TiltRightSpin->setValue(TRACKER.axisGet(ROLL, AXIS_RMULT));
      break;
    case YAW:
      if((elem == AXIS_ENABLED) || (elem == AXIS_FULL)) gui.YawEnable->setCheckState(bool2state(TRACKER.axisGetEnabled(YAW)));
      if((elem == AXIS_LMULT) || (elem == AXIS_FULL)) gui.YawLeftSpin->setValue(TRACKER.axisGet(YAW, AXIS_LMULT));
      if((elem == AXIS_RMULT) || (elem == AXIS_FULL)) gui.YawRightSpin->setValue(TRACKER.axisGet(YAW, AXIS_RMULT));
      break;
    case TX:
      if((elem == AXIS_ENABLED) || (elem == AXIS_FULL)) gui.XEnable->setCheckState(bool2state(TRACKER.axisGetEnabled(TX)));
      if((elem == AXIS_LMULT) || (elem == AXIS_FULL)) gui.MoveLeftSpin->setValue(TRACKER.axisGet(TX, AXIS_LMULT));
      if((elem == AXIS_RMULT) || (elem == AXIS_FULL)) gui.MoveRightSpin->setValue(TRACKER.axisGet(TX, AXIS_RMULT));
      break;
    case TY:
      if((elem == AXIS_ENABLED) || (elem == AXIS_FULL)) gui.YEnable->setCheckState(bool2state(TRACKER.axisGetEnabled(TY)));
      if((elem == AXIS_LMULT) || (elem == AXIS_FULL)) gui.MoveUpSpin->setValue(TRACKER.axisGet(TY, AXIS_LMULT));
      if((elem == AXIS_RMULT) || (elem == AXIS_FULL)) gui.MoveDownSpin->setValue(TRACKER.axisGet(TY, AXIS_RMULT));
      break;
    case TZ:
      if((elem == AXIS_ENABLED) || (elem == AXIS_FULL)) gui.ZEnable->setCheckState(bool2state(TRACKER.axisGetEnabled(TZ)));
      if((elem == AXIS_LMULT) || (elem == AXIS_FULL)) gui.MoveBackSpin->setValue(TRACKER.axisGet(TZ, AXIS_LMULT));
      if((elem == AXIS_RMULT) || (elem == AXIS_FULL)) gui.MoveForthSpin->setValue(TRACKER.axisGet(TZ, AXIS_RMULT));
      break;
    default:
      break;
  }
}

void LtrTracking::on_Profiles_currentIndexChanged(const QString &text)
{
  bool prev = initializing;
  initializing = true;
  //std::cout<<"Index changed to "<<text.toStdString()<<std::endl;
  PROFILE.setCurrent(text);
  emit customSectionChanged();
  initializing = prev;
}

void LtrTracking::on_CreateNewProfile_pressed()
{
  bool done;
  QString newSec;
  newSec = QInputDialog::getText(NULL, "New Secion Name:", 
                        "Enter name of the new section:", 
                        QLineEdit::Normal, "", &done);
  if(done && !newSec.isEmpty()){
    int i = PROFILE.isProfile(newSec);
    if(i == -1){
      PROFILE.addProfile(newSec);
      gui.Profiles->clear();
      const QStringList &sl = Profile::getProfiles().getProfileNames();
      gui.Profiles->addItems(sl);
      gui.Profiles->setCurrentIndex(sl.size() - 1);
    }else{
      gui.Profiles->setCurrentIndex(i);
    }
  }
}

static bool state2bool(int state)
{
  if(state == Qt::Checked){
    return true;
  }else{
    return false;
  }
}

void LtrTracking::on_PitchEnable_stateChanged(int state)
{
  if(!initializing) TRACKER.axisChangeEnabled(PITCH, state2bool(state));
}

void LtrTracking::on_RollEnable_stateChanged(int state)
{
  if(!initializing) TRACKER.axisChangeEnabled(ROLL, state2bool(state));
}

void LtrTracking::on_YawEnable_stateChanged(int state)
{
  if(!initializing) TRACKER.axisChangeEnabled(YAW, state2bool(state));
}

void LtrTracking::on_XEnable_stateChanged(int state)
{
  if(!initializing) TRACKER.axisChangeEnabled(TX, state2bool(state));
}

void LtrTracking::on_YEnable_stateChanged(int state)
{
  if(!initializing) TRACKER.axisChangeEnabled(TY, state2bool(state));
}

void LtrTracking::on_ZEnable_stateChanged(int state)
{
  if(!initializing) TRACKER.axisChangeEnabled(TZ, state2bool(state));
}

void LtrTracking::on_PitchUpSpin_valueChanged(double d)
{
  if(!initializing) TRACKER.axisChange(PITCH, AXIS_LMULT, d);
}

void LtrTracking::on_PitchDownSpin_valueChanged(double d)
{
  if(!initializing) TRACKER.axisChange(PITCH, AXIS_RMULT, d);
}

void LtrTracking::on_YawLeftSpin_valueChanged(double d)
{
  if(!initializing) TRACKER.axisChange(YAW, AXIS_LMULT, d);
}

void LtrTracking::on_YawRightSpin_valueChanged(double d)
{
  if(!initializing) TRACKER.axisChange(YAW, AXIS_RMULT, d);
}

void LtrTracking::on_TiltLeftSpin_valueChanged(double d)
{
  if(!initializing) TRACKER.axisChange(ROLL, AXIS_LMULT, d);
}

void LtrTracking::on_TiltRightSpin_valueChanged(double d)
{
  if(!initializing) TRACKER.axisChange(ROLL, AXIS_RMULT, d);
}

void LtrTracking::on_MoveLeftSpin_valueChanged(double d)
{
  if(!initializing) TRACKER.axisChange(TX, AXIS_LMULT, d);
}

void LtrTracking::on_MoveRightSpin_valueChanged(double d)
{
  if(!initializing) TRACKER.axisChange(TX, AXIS_RMULT, d);
}

void LtrTracking::on_MoveUpSpin_valueChanged(double d)
{
  if(!initializing) TRACKER.axisChange(TY, AXIS_LMULT, d);
}

void LtrTracking::on_MoveDownSpin_valueChanged(double d)
{
  if(!initializing) TRACKER.axisChange(TY, AXIS_RMULT, d);
}

void LtrTracking::on_MoveBackSpin_valueChanged(double d)
{
  if(!initializing) TRACKER.axisChange(TZ, AXIS_LMULT, d);
}

void LtrTracking::on_MoveForthSpin_valueChanged(double d)
{
  if(!initializing) TRACKER.axisChange(TZ, AXIS_RMULT, d);
}

static Qt::CheckState bool2state(bool v)
{
  if(v){
    return Qt::Checked;
  }else{
    return Qt::Unchecked;
  }
}

void LtrTracking::setCommonFFVal(float val)
{
  gui.CommonFFVal->setText(QString("%1%").arg(val * 100.0, 5, 'f', 1));
}

void LtrTracking::on_CommonFF_valueChanged(int value)
{
  float val = ((float)value)/gui.CommonFF->maximum();
  if(!initializing){
    TRACKER.setCommonFilterFactor(val);
  }
  setCommonFFVal(val);
}

void LtrTracking::setCommonFF(float val)
{
  gui.CommonFF->setValue(val * gui.CommonFF->maximum());
  setCommonFFVal(val);
}

void LtrTracking::initAxes()
{
  initializing = true;
  gui.PitchEnable->setCheckState(bool2state(TRACKER.axisGetEnabled(PITCH)));
  gui.PitchUpSpin->setValue(TRACKER.axisGet(PITCH, AXIS_LMULT));
  gui.PitchDownSpin->setValue(TRACKER.axisGet(PITCH, AXIS_RMULT));
  gui.RollEnable->setCheckState(bool2state(TRACKER.axisGetEnabled(ROLL)));
  gui.TiltLeftSpin->setValue(TRACKER.axisGet(ROLL, AXIS_LMULT));
  gui.TiltRightSpin->setValue(TRACKER.axisGet(ROLL, AXIS_RMULT));
  gui.YawEnable->setCheckState(bool2state(TRACKER.axisGetEnabled(YAW)));
  gui.YawLeftSpin->setValue(TRACKER.axisGet(YAW, AXIS_LMULT));
  gui.YawRightSpin->setValue(TRACKER.axisGet(YAW, AXIS_RMULT));
  gui.XEnable->setCheckState(bool2state(TRACKER.axisGetEnabled(TX)));
  gui.MoveLeftSpin->setValue(TRACKER.axisGet(TX, AXIS_LMULT));
  gui.MoveRightSpin->setValue(TRACKER.axisGet(TX, AXIS_RMULT));
  gui.YEnable->setCheckState(bool2state(TRACKER.axisGetEnabled(TY)));
  gui.MoveUpSpin->setValue(TRACKER.axisGet(TY, AXIS_LMULT));
  gui.MoveDownSpin->setValue(TRACKER.axisGet(TY, AXIS_RMULT));
  gui.ZEnable->setCheckState(bool2state(TRACKER.axisGetEnabled(TZ)));
  gui.MoveBackSpin->setValue(TRACKER.axisGet(TZ, AXIS_LMULT));
  gui.MoveForthSpin->setValue(TRACKER.axisGet(TZ, AXIS_RMULT));
  setCommonFF(TRACKER.getCommonFilterFactor());
  initializing = false;
}

