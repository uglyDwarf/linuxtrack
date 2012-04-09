#include <iostream>
#include "scurve.h"
#include "pref_global.h"
#include "ltr_gui_prefs.h"
#include "ltr_gui.h"
#include "ltr_profiles.h"
#include "tracker.h"
#include <math.h>

SCurve::SCurve(axis_t a, QString axis_name, QString left_label, QString right_label, QWidget *parent)
  : QWidget(parent), axis(a), symetrical(true), view(NULL), first(true)
{
  symetrical = TRACKER.axisIsSymetrical(axis);
  ui.setupUi(this);
  ui.SCTitle->setText(axis_name);
  ui.SCLeftLabel->setText(left_label);
  ui.SCRightLabel->setText(right_label);
  QObject::connect(&TRACKER, SIGNAL(axisChanged(int, int)),
                    this, SLOT(axisChanged(int, int)));
  
  first = false;
  view = new SCView(axis, this);
  ui.SCView->removeItem(ui.SCViewSpacer);
  ui.SCView->addWidget(view);
  QObject::connect(this, SIGNAL(changed()), view, SLOT(update()));
  initializing = true;
  initializing = false;
}

SCurve::~SCurve()
{
  delete view;
}

void SCurve::setup_gui()
{
  if(TRACKER.axisIsSymetrical(axis)){
    ui.SCSymetrical->setCheckState(Qt::Checked);
    symetrical = true;
  }else{
    ui.SCSymetrical->setCheckState(Qt::Unchecked);
    symetrical = false;
  }
  ui.SCLeftFactor->setValue(TRACKER.axisGet(axis, AXIS_LMULT));
  ui.SCLeftCurv->setValue(TRACKER.axisGet(axis, AXIS_LCURV) * 100);
  ui.SCRightFactor->setValue(TRACKER.axisGet(axis, AXIS_RMULT));
  ui.SCRightCurv->setValue(TRACKER.axisGet(axis, AXIS_RCURV) * 100);
  ui.SCDeadZone->setValue(TRACKER.axisGet(axis, AXIS_DEADZONE) * 101);
  ui.SCRightLimit->setValue(TRACKER.axisGet(axis, AXIS_LLIMIT));
  ui.SCLeftLimit->setValue(TRACKER.axisGet(axis, AXIS_RLIMIT));
  ui.SCCurvL->setText(QString("Curvature: %1").arg(TRACKER.axisGet(axis, AXIS_LCURV), 2, 'f', 2));
  ui.SCCurvR->setText(QString("Curvature: %1").arg(TRACKER.axisGet(axis, AXIS_RCURV), 2, 'f', 2));
  ui.SCDZoneLabel->setText(QString("DeadZone: %1").arg(TRACKER.axisGet(axis, AXIS_DEADZONE), 2, 'f', 2));
}

void SCurve::setEnabled(int state)
{
  if(state == Qt::Checked){
    //std::cout<<"Enabling..."<<std::endl;
    if(!initializing) TRACKER.axisChangeEnabled(axis, true);
  }else{
    //std::cout<<"Disabling..."<<std::endl;
    if(!initializing) TRACKER.axisChangeEnabled(axis, false);
  }
}

void SCurve::on_SCSymetrical_stateChanged(int state)
{
  switch(state){
    case Qt::Checked:
      symetrical = true;
      ui.SCRightFactor->setDisabled(true);
      ui.SCRightFactor->setValue(ui.SCLeftFactor->value());
      ui.SCRightLimit->setDisabled(true);
      ui.SCRightLimit->setValue(ui.SCLeftLimit->value());
      ui.SCRightCurv->setDisabled(true);
      ui.SCRightCurv->setValue(ui.SCLeftCurv->value());
      break;
    case Qt::Unchecked:
      symetrical = false;
      ui.SCRightFactor->setDisabled(false);
      ui.SCRightLimit->setDisabled(false);
      ui.SCRightCurv->setDisabled(false);
      break;
    default:
      break;
  }
}

void SCurve::on_SCLeftFactor_valueChanged(double d)
{
  //std::cout<<"LeftFactor = "<<d<<std::endl;
  if(!initializing) TRACKER.axisChange(axis, AXIS_LMULT, d);
  if(symetrical){
    ui.SCRightFactor->setValue(d);
  }
  emit changed();
}

void SCurve::on_SCRightFactor_valueChanged(double d)
{
  //std::cout<<"RightFactor = "<<d<<std::endl;
  if(!initializing) TRACKER.axisChange(axis, AXIS_RMULT, d);
  if(symetrical){
    ui.SCLeftFactor->setValue(d);
  }
  emit changed();
}

void SCurve::on_SCLeftCurv_valueChanged(int value)
{
  //std::cout<<"LeftCurv = "<<value<<std::endl;
  if(!initializing) TRACKER.axisChange(axis, AXIS_LCURV, value / 100.0);
  ui.SCCurvL->setText(QString("Curvature: %1").arg(value / 100.0, 2, 'f', 2));
  if(symetrical){
    ui.SCRightCurv->setValue(value);
  }else{
    emit changed();
  }
}

void SCurve::on_SCRightCurv_valueChanged(int value)
{
  //std::cout<<"RightCurv = "<<value<<std::endl;
  if(!initializing) TRACKER.axisChange(axis, AXIS_RCURV, value / 100.0);
  ui.SCCurvR->setText(QString("Curvature: %1").arg(value / 100.0, 2, 'f', 2));
  emit changed();
}

void SCurve::on_SCDeadZone_valueChanged(int value)
{
  //std::cout<<"DeadZone = "<<value<<std::endl;
  if(!initializing) TRACKER.axisChange(axis, AXIS_DEADZONE, value / 101.0);
  ui.SCDZoneLabel->setText(QString("DeadZone: %1").arg(value / 101.0, 2, 'f', 2));
  emit changed();
}

void SCurve::on_SCLeftLimit_valueChanged(double d)
{
  //std::cout<<"LLimit = "<<d<<std::endl;
  if(!initializing) TRACKER.axisChange(axis, AXIS_LLIMIT, d);
  if(symetrical){
    ui.SCRightLimit->setValue(d);
  }
  emit changed();
}

void SCurve::on_SCRightLimit_valueChanged(double d)
{
  //std::cout<<"RLimit = "<<d<<std::endl;
  if(!initializing) TRACKER.axisChange(axis, AXIS_RLIMIT, d);
  if(symetrical){
    ui.SCLeftLimit->setValue(d);
  }
  emit changed();
}

//void SCurve::movePoint(float new_x)
//{
/*
  float val = new_x / axis->getLimits();
  if(val > 1.0f){
    val = 1.0f;
  }
  if(val < -1.0f){
    val = -1.0f;
  }
  view->movePoint(val);
*/
//  view->movePoint(new_x);
//}

void SCurve::axisChanged(int a, int elem)
{
  if(a != axis){
    return;
  }
  switch(elem){
    case AXIS_LMULT:
      ui.SCLeftFactor->setValue(TRACKER.axisGet(axis, AXIS_LMULT));
      break;
    case AXIS_RMULT:
      ui.SCRightFactor->setValue(TRACKER.axisGet(axis, AXIS_RMULT));
      break;
    case AXIS_LCURV:
      ui.SCLeftCurv->setValue(round(TRACKER.axisGet(axis, AXIS_LCURV) * 100.0));
      ui.SCCurvL->setText(QString("Curvature: %1").arg(TRACKER.axisGet(axis, AXIS_LCURV), 2, 'f', 2));
      break;
    case AXIS_RCURV:
      ui.SCRightCurv->setValue(round(TRACKER.axisGet(axis, AXIS_RCURV) * 100.0));
      ui.SCCurvR->setText(QString("Curvature: %1").arg(TRACKER.axisGet(axis, AXIS_RCURV), 2, 'f', 2));
      break;
    case AXIS_DEADZONE:
      ui.SCDeadZone->setValue(round(TRACKER.axisGet(axis, AXIS_DEADZONE) * 101.0));
      ui.SCDZoneLabel->setText(QString("DeadZone: %1").arg(TRACKER.axisGet(axis, AXIS_DEADZONE), 2, 'f', 2));
      break;
    case AXIS_LLIMIT:
      ui.SCLeftLimit->setValue(TRACKER.axisGet(axis, AXIS_LLIMIT));
      break;
    case AXIS_RLIMIT:
      ui.SCRightLimit->setValue(TRACKER.axisGet(axis, AXIS_RLIMIT));
      break;
    case AXIS_ENABLED:
      if(TRACKER.axisIsSymetrical(axis)){
	ui.SCSymetrical->setCheckState(Qt::Checked);
        symetrical = true;
      }else{
	ui.SCSymetrical->setCheckState(Qt::Unchecked);
        symetrical = false;
      }
      break;
    case AXIS_FULL:
      ui.SCLeftFactor->setValue(TRACKER.axisGet(axis, AXIS_LMULT));
      ui.SCRightFactor->setValue(TRACKER.axisGet(axis, AXIS_RMULT));
      ui.SCLeftCurv->setValue(round(TRACKER.axisGet(axis, AXIS_LCURV) * 100.0));
      ui.SCRightCurv->setValue(round(TRACKER.axisGet(axis, AXIS_RCURV) * 100.0));
      ui.SCDeadZone->setValue(round(TRACKER.axisGet(axis, AXIS_DEADZONE) * 101.0));
      ui.SCCurvL->setText(QString("Curvature: %1").arg(TRACKER.axisGet(axis, AXIS_LCURV), 2, 'f', 2));
      ui.SCCurvR->setText(QString("Curvature: %1").arg(TRACKER.axisGet(axis, AXIS_RCURV), 2, 'f', 2));
      ui.SCDZoneLabel->setText(QString("DeadZone: %1").arg(TRACKER.axisGet(axis, AXIS_DEADZONE), 2, 'f', 2));
      ui.SCLeftLimit->setValue(TRACKER.axisGet(axis, AXIS_LLIMIT));
      ui.SCRightLimit->setValue(TRACKER.axisGet(axis, AXIS_RLIMIT));
      if(TRACKER.axisIsSymetrical(axis)){
	ui.SCSymetrical->setCheckState(Qt::Checked);
        symetrical = true;
      }else{
	ui.SCSymetrical->setCheckState(Qt::Unchecked);
        symetrical = false;
      }
      break;
    default:
      break;
  }
  emit changed();
}

