#include "ltr_tracking.h"
#include "ltr_profiles.h"
#include <QInputDialog>
#include <iostream>

LtrTracking::LtrTracking(const Ui::LinuxtrackMainForm &ui) : gui(ui)
{
  ffChanged(PROFILE.getCurrentProfile()->getFilterFactor());
  pitch = PROFILE.getCurrentProfile()->getPitchAxis();
  roll = PROFILE.getCurrentProfile()->getRollAxis();
  yaw = PROFILE.getCurrentProfile()->getYawAxis();
  tx = PROFILE.getCurrentProfile()->getTxAxis();
  ty = PROFILE.getCurrentProfile()->getTyAxis();
  tz = PROFILE.getCurrentProfile()->getTzAxis();
  Connect();
  gui.Profiles->addItems(Profile::getProfiles().getProfileNames());
}

LtrTracking::~LtrTracking()
{
}

void LtrTracking::Connect()
{
  QObject::connect(PROFILE.getCurrentProfile(), 
                    SIGNAL(filterFactorChanged(float)),this, SLOT(ffChanged(float)));
  QObject::connect(gui.FilterSlider, SIGNAL(valueChanged(int)), 
                    this, SLOT(on_FilterSlider_valueChanged(int)));
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
  QObject::connect(pitch, SIGNAL(axisChanged(AxisElem_t)),
                    this, SLOT(pitchChanged(AxisElem_t)));
  QObject::connect(roll, SIGNAL(axisChanged(AxisElem_t)),
                    this, SLOT(rollChanged(AxisElem_t)));
  QObject::connect(yaw, SIGNAL(axisChanged(AxisElem_t)),
                    this, SLOT(yawChanged(AxisElem_t)));
  QObject::connect(tx, SIGNAL(axisChanged(AxisElem_t)),
                    this, SLOT(txChanged(AxisElem_t)));
  QObject::connect(ty, SIGNAL(axisChanged(AxisElem_t)),
                    this, SLOT(tyChanged(AxisElem_t)));
  QObject::connect(tz, SIGNAL(axisChanged(AxisElem_t)),
                    this, SLOT(tzChanged(AxisElem_t)));
//  QObject::connect(, SIGNAL(), , SLOT());
}

void LtrTracking::ffChanged(float f)
{
  gui.FilterValue->setText(QString::number(f));
  gui.FilterSlider->setValue(f * 10 + 1);
}

void LtrTracking::on_FilterSlider_valueChanged(int value)
{
  gui.FilterValue->setText(QString::number(value / 10.0f));
  PROFILE.getCurrentProfile()->setFilterFactor(value / 10.0f);
}

void LtrTracking::on_Profiles_currentIndexChanged(const QString &text)
{
  PROFILE.setCurrent(text);
  emit customSectionChanged();
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

bool state2bool(int state)
{
  if(state == Qt::Checked){
    return true;
  }else{
    return false;
  }
}

void LtrTracking::on_PitchEnable_stateChanged(int state)
{
  pitch->changeEnabled((state2bool(state)));
}

void LtrTracking::on_RollEnable_stateChanged(int state)
{
  roll->changeEnabled((state2bool(state)));
}

void LtrTracking::on_YawEnable_stateChanged(int state)
{
  yaw->changeEnabled((state2bool(state)));
}

void LtrTracking::on_XEnable_stateChanged(int state)
{
  tx->changeEnabled((state2bool(state)));
}

void LtrTracking::on_YEnable_stateChanged(int state)
{
  ty->changeEnabled((state2bool(state)));
}

void LtrTracking::on_ZEnable_stateChanged(int state)
{
  tz->changeEnabled((state2bool(state)));
}

Qt::CheckState bool2state(bool v)
{
  if(v){
    return Qt::Checked;
  }else{
    return Qt::Unchecked;
  }
}

void LtrTracking::pitchChanged(AxisElem_t what)
{
  switch(what){
    case ENABLED:
      gui.PitchEnable->setCheckState(bool2state(pitch->getEnabled()));
      break;
    case RELOAD:
      gui.PitchEnable->setCheckState(bool2state(pitch->getEnabled()));
      break;
    default:
      break;
  }
}

void LtrTracking::rollChanged(AxisElem_t what)
{
  switch(what){
    case ENABLED:
      gui.RollEnable->setCheckState(bool2state(roll->getEnabled()));
      break;
    case RELOAD:
      gui.RollEnable->setCheckState(bool2state(roll->getEnabled()));
      break;
    default:
      break;
  }
}

void LtrTracking::yawChanged(AxisElem_t what)
{
  switch(what){
    case ENABLED:
      gui.YawEnable->setCheckState(bool2state(yaw->getEnabled()));
      break;
    case RELOAD:
      gui.YawEnable->setCheckState(bool2state(yaw->getEnabled()));
      break;
    default:
      break;
  }
}

void LtrTracking::txChanged(AxisElem_t what)
{
  switch(what){
    case ENABLED:
      gui.XEnable->setCheckState(bool2state(tx->getEnabled()));
      break;
    case RELOAD:
      gui.XEnable->setCheckState(bool2state(tx->getEnabled()));
      break;
    default:
      break;
  }
}

void LtrTracking::tyChanged(AxisElem_t what)
{
  switch(what){
    case ENABLED:
      gui.YEnable->setCheckState(bool2state(ty->getEnabled()));
      break;
    case RELOAD:
      gui.YEnable->setCheckState(bool2state(ty->getEnabled()));
      break;
    default:
      break;
  }
}

void LtrTracking::tzChanged(AxisElem_t what)
{
  switch(what){
    case ENABLED:
      gui.ZEnable->setCheckState(bool2state(tz->getEnabled()));
      break;
    case RELOAD:
      gui.ZEnable->setCheckState(bool2state(tz->getEnabled()));
      break;
    default:
      break;
  }
}
