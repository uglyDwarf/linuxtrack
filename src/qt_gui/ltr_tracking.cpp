#include <ltr_tracking.h>
#include <ltr_profiles.h>
#include <ltr_gui_prefs.h>
#include <QInputDialog>
#include <iostream>

LtrTracking::LtrTracking(const Ui::LinuxtrackMainForm &ui) : gui(ui), initializing(false)
{
  initializing = true;
  ffChanged(PROFILE.getCurrentProfile()->getFilterFactor());
  pitch = PROFILE.getCurrentProfile()->getPitchAxis();
  roll = PROFILE.getCurrentProfile()->getRollAxis();
  yaw = PROFILE.getCurrentProfile()->getYawAxis();
  tx = PROFILE.getCurrentProfile()->getTxAxis();
  ty = PROFILE.getCurrentProfile()->getTyAxis();
  tz = PROFILE.getCurrentProfile()->getTzAxis();
  initializing = false;
  Connect();
  gui.Profiles->addItems(Profile::getProfiles().getProfileNames());
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
}

void LtrTracking::ffChanged(float f)
{
  gui.FilterValue->setText(QString::number(f));
  gui.FilterSlider->setValue(f * 10);
  if(!initializing) PREF.setKeyVal(PROFILE.getCurrentProfile()->getProfileName(), "Filter-factor", f);
}

void LtrTracking::on_FilterSlider_valueChanged(int value)
{
  gui.FilterValue->setText(QString::number(value / 10.0f));
  PROFILE.getCurrentProfile()->setFilterFactor(value / 10.0f);
}

void LtrTracking::on_Profiles_currentIndexChanged(const QString &text)
{
  initializing = true;
  PROFILE.setCurrent(text);
  emit customSectionChanged();
  initializing = false;
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
  if(!initializing) pitch->changeEnabled((state2bool(state)));
}

void LtrTracking::on_RollEnable_stateChanged(int state)
{
  if(!initializing) roll->changeEnabled((state2bool(state)));
}

void LtrTracking::on_YawEnable_stateChanged(int state)
{
  if(!initializing) yaw->changeEnabled((state2bool(state)));
}

void LtrTracking::on_XEnable_stateChanged(int state)
{
  if(!initializing) tx->changeEnabled((state2bool(state)));
}

void LtrTracking::on_YEnable_stateChanged(int state)
{
  if(!initializing) ty->changeEnabled((state2bool(state)));
}

void LtrTracking::on_ZEnable_stateChanged(int state)
{
  if(!initializing) tz->changeEnabled((state2bool(state)));
}

void LtrTracking::on_PitchUpSpin_valueChanged(double d)
{
  if(!initializing) pitch->changeLFactor(d);
}

void LtrTracking::on_PitchDownSpin_valueChanged(double d)
{
  if(!initializing) pitch->changeRFactor(d);
}

void LtrTracking::on_YawLeftSpin_valueChanged(double d)
{
  if(!initializing) yaw->changeLFactor(d);
}

void LtrTracking::on_YawRightSpin_valueChanged(double d)
{
  if(!initializing) yaw->changeRFactor(d);
}

void LtrTracking::on_TiltLeftSpin_valueChanged(double d)
{
  if(!initializing) roll->changeLFactor(d);
}

void LtrTracking::on_TiltRightSpin_valueChanged(double d)
{
  if(!initializing) roll->changeRFactor(d);
}

void LtrTracking::on_MoveLeftSpin_valueChanged(double d)
{
  if(!initializing) tx->changeLFactor(d);
}

void LtrTracking::on_MoveRightSpin_valueChanged(double d)
{
  if(!initializing) tx->changeRFactor(d);
}

void LtrTracking::on_MoveUpSpin_valueChanged(double d)
{
  if(!initializing) ty->changeLFactor(d);
}

void LtrTracking::on_MoveDownSpin_valueChanged(double d)
{
  if(!initializing) ty->changeRFactor(d);
}

void LtrTracking::on_MoveBackSpin_valueChanged(double d)
{
  if(!initializing) tz->changeLFactor(d);
}

void LtrTracking::on_MoveForthSpin_valueChanged(double d)
{
  if(!initializing) tz->changeRFactor(d);
}

static Qt::CheckState bool2state(bool v)
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
    case LFACTOR:
      gui.PitchUpSpin->setValue(pitch->getLFactor());
      break;
    case RFACTOR:
      gui.PitchDownSpin->setValue(pitch->getRFactor());
      break;
    case RELOAD:
      gui.PitchEnable->setCheckState(bool2state(pitch->getEnabled()));
      gui.PitchUpSpin->setValue(pitch->getLFactor());
      gui.PitchDownSpin->setValue(pitch->getRFactor());
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
    case LFACTOR:
      gui.TiltLeftSpin->setValue(roll->getLFactor());
      break;
    case RFACTOR:
      gui.TiltRightSpin->setValue(roll->getRFactor());
      break;
    case RELOAD:
      gui.RollEnable->setCheckState(bool2state(roll->getEnabled()));
      gui.TiltLeftSpin->setValue(roll->getLFactor());
      gui.TiltRightSpin->setValue(roll->getRFactor());
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
    case LFACTOR:
      gui.YawLeftSpin->setValue(yaw->getLFactor());
      break;
    case RFACTOR:
      gui.YawRightSpin->setValue(yaw->getRFactor());
      break;
    case RELOAD:
      gui.YawEnable->setCheckState(bool2state(yaw->getEnabled()));
      gui.YawLeftSpin->setValue(yaw->getLFactor());
      gui.YawRightSpin->setValue(yaw->getRFactor());
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
    case LFACTOR:
      gui.MoveLeftSpin->setValue(tx->getLFactor());
      break;
    case RFACTOR:
      gui.MoveRightSpin->setValue(tx->getRFactor());
      break;
    case RELOAD:
      gui.XEnable->setCheckState(bool2state(tx->getEnabled()));
      gui.MoveLeftSpin->setValue(tx->getLFactor());
      gui.MoveRightSpin->setValue(tx->getRFactor());
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
    case LFACTOR:
      gui.MoveUpSpin->setValue(ty->getLFactor());
      break;
    case RFACTOR:
      gui.MoveDownSpin->setValue(ty->getRFactor());
      break;
    case RELOAD:
      gui.YEnable->setCheckState(bool2state(ty->getEnabled()));
      gui.MoveUpSpin->setValue(ty->getLFactor());
      gui.MoveDownSpin->setValue(ty->getRFactor());
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
    case LFACTOR:
      gui.MoveBackSpin->setValue(tz->getLFactor());
      break;
    case RFACTOR:
      gui.MoveForthSpin->setValue(tz->getRFactor());
      break;
    case RELOAD:
      gui.ZEnable->setCheckState(bool2state(tz->getEnabled()));
      gui.MoveBackSpin->setValue(tz->getLFactor());
      gui.MoveForthSpin->setValue(tz->getRFactor());
      break;
    default:
      break;
  }
}
