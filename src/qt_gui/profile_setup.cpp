#include <QMessageBox>
#include "profile_setup.h"
#include "scp_form.h"
#include "tracker.cpp"


ProfileSetup::ProfileSetup(const QString &name, QWidget *parent) : QWidget(parent), sc(NULL), profileName(name),
  initializing(true)
{
  ui.setupUi(this);
  connect();
  sc = new ScpForm();
  TRACKER.setProfile(profileName);
  initializing = false;
}


ProfileSetup::~ProfileSetup()
{
  delete sc;
}

void ProfileSetup::connect()
{
  QObject::connect(&TRACKER, SIGNAL(initAxes(void)), this, SLOT(initAxes(void)));
  QObject::connect(&TRACKER, SIGNAL(axisChanged(int, int)), this, SLOT(axisChanged(int, int)));
  QObject::connect(&TRACKER, SIGNAL(setCommonFF(float)), this, SLOT(setCommonFF(float)));
}

void ProfileSetup::on_DetailedAxisSetup_pressed()
{
  sc->show();
}

void ProfileSetup::on_CopyFromDefault_pressed()
{
  if(QMessageBox::warning(this, "Warning:", 
        "Are you sure, you want to overwrite the current profile by default values?", 
        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Ok){
    return;
  }
  TRACKER.fromDefault();
}

void ProfileSetup::initAxes()
{
  ui.PitchEnable->setCheckState(TRACKER.axisGetEnabled(PITCH)?Qt::Checked:Qt::Unchecked);
  ui.YawEnable->setCheckState(TRACKER.axisGetEnabled(ROLL)?Qt::Checked:Qt::Unchecked);
  ui.RollEnable->setCheckState(TRACKER.axisGetEnabled(YAW)?Qt::Checked:Qt::Unchecked);
  ui.TxEnable->setCheckState(TRACKER.axisGetEnabled(TX)?Qt::Checked:Qt::Unchecked);
  ui.TyEnable->setCheckState(TRACKER.axisGetEnabled(TY)?Qt::Checked:Qt::Unchecked);
  ui.TzEnable->setCheckState(TRACKER.axisGetEnabled(TZ)?Qt::Checked:Qt::Unchecked);
  ui.PitchSens->setValue(TRACKER.axisGet(PITCH, AXIS_MULT) * 12.0);
  ui.RollSens->setValue(TRACKER.axisGet(ROLL, AXIS_MULT) * 12.0);
  ui.YawSens->setValue(TRACKER.axisGet(YAW, AXIS_MULT) * 12.0);
  ui.TxSens->setValue(TRACKER.axisGet(TX, AXIS_MULT) * 12.0);
  ui.TySens->setValue(TRACKER.axisGet(TY, AXIS_MULT) * 12.0);
  ui.TzSens->setValue(TRACKER.axisGet(TZ, AXIS_MULT) * 12.0);
  ui.Smoothing->setValue(TRACKER.getCommonFilterFactor() * ui.Smoothing->maximum());
}



void ProfileSetup::axisChanged(int axis, int elem)
{
  if(elem == AXIS_MULT){
    switch(axis){
      case PITCH:
        ui.PitchSens->setValue(TRACKER.axisGet(PITCH, AXIS_MULT) * 12.0);
        break;
      case YAW:
        ui.YawSens->setValue(TRACKER.axisGet(YAW, AXIS_MULT) * 12.0);
        break;
      case ROLL:
        ui.RollSens->setValue(TRACKER.axisGet(ROLL, AXIS_MULT) * 12.0);
        break;
      case TX:
        ui.TxSens->setValue(TRACKER.axisGet(TX, AXIS_MULT) * 12.0);
        break;
      case TY:
        ui.TySens->setValue(TRACKER.axisGet(TY, AXIS_MULT) * 12.0);
        break;
      case TZ:
        ui.TzSens->setValue(TRACKER.axisGet(TZ, AXIS_MULT) * 12.0);
        break;
    }
  }
}

void ProfileSetup::setCommonFF(float val)
{
  ui.Smoothing->setValue(val * ui.Smoothing->maximum());
}

void ProfileSetup::on_PitchEnable_stateChanged(int state)
{
  if(!initializing) TRACKER.axisChangeEnabled(PITCH, (state == Qt::Checked));
}

void ProfileSetup::on_YawEnable_stateChanged(int state)
{
  if(!initializing) TRACKER.axisChangeEnabled(YAW, (state == Qt::Checked));
}

void ProfileSetup::on_RollEnable_stateChanged(int state)
{
  if(!initializing) TRACKER.axisChangeEnabled(ROLL, (state == Qt::Checked));
}

void ProfileSetup::on_TxEnable_stateChanged(int state)
{
  if(!initializing) TRACKER.axisChangeEnabled(TX, (state == Qt::Checked));
}

void ProfileSetup::on_TyEnable_stateChanged(int state)
{
  if(!initializing) TRACKER.axisChangeEnabled(TY, (state == Qt::Checked));
}

void ProfileSetup::on_TzEnable_stateChanged(int state)
{
  if(!initializing) TRACKER.axisChangeEnabled(TZ, (state == Qt::Checked));
}

void ProfileSetup::on_PitchSens_valueChanged(int val)
{
  if(!initializing) TRACKER.axisChange(PITCH, AXIS_MULT, val / 12.0);
}

void ProfileSetup::on_YawSens_valueChanged(int val)
{
  if(!initializing) TRACKER.axisChange(YAW, AXIS_MULT, val / 12.0);
}

void ProfileSetup::on_RollSens_valueChanged(int val)
{
  if(!initializing) TRACKER.axisChange(ROLL, AXIS_MULT, val / 12.0);
}

void ProfileSetup::on_TxSens_valueChanged(int val)
{
  if(!initializing) TRACKER.axisChange(TX, AXIS_MULT, val / 12.0);
}

void ProfileSetup::on_TySens_valueChanged(int val)
{
  if(!initializing) TRACKER.axisChange(TY, AXIS_MULT, val / 12.0);
}

void ProfileSetup::on_TzSens_valueChanged(int val)
{
  if(!initializing) TRACKER.axisChange(TZ, AXIS_MULT, val / 12.0);
}

void ProfileSetup::on_Smoothing_valueChanged(int val)
{
  if(!initializing) TRACKER.setCommonFilterFactor((float)val / ui.Smoothing->maximum());
}

