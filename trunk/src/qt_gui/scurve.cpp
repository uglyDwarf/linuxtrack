#include <iostream>
#include "scurve.h"
#include "pref_global.h"
#include "ltr_gui_prefs.h"
#include "ltr_gui.h"
#include "ltr_profiles.h"
#include "ltr_axis.h"
#include <math.h>

SCurve::SCurve(LtrAxis *a, QString axis_name, QString left_label, QString right_label, QWidget *parent)
  : QWidget(parent), axis(a), symetrical(true), view(NULL), first(true)
{
  symetrical = axis->isSymetrical();
  ui.setupUi(this);
  ui.SCTitle->setText(axis_name);
  ui.SCLeftLabel->setText(left_label);
  ui.SCRightLabel->setText(right_label);
  QObject::connect(axis, SIGNAL(axisChanged(AxisElem_t)),
                    this, SLOT(axisChanged(AxisElem_t)));
  
  first = false;
  view = new SCView(axis, this);
  ui.SCView->removeItem(ui.SCViewSpacer);
  ui.SCView->addWidget(view);
  QObject::connect(this, SIGNAL(changed()), view, SLOT(update()));
  initializing = true;
  axisChanged(RELOAD);
  initializing = false;
}

SCurve::~SCurve()
{
  delete view;
}

void SCurve::setup_gui()
{
  if(axis->isSymetrical()){
    ui.SCSymetrical->setCheckState(Qt::Checked);
    symetrical = true;
  }else{
    ui.SCSymetrical->setCheckState(Qt::Unchecked);
    symetrical = false;
  }
  ui.SCLeftFactor->setValue(axis->getLFactor());
  ui.SCLeftCurv->setValue(axis->getLCurv() * 100);
  ui.SCRightFactor->setValue(axis->getRFactor());
  ui.SCRightCurv->setValue(axis->getRCurv() * 100);
  ui.SCDeadZone->setValue(axis->getDZone() * 101);
  ui.SCRightLimit->setValue(axis->getLLimit());
  ui.SCLeftLimit->setValue(axis->getRLimit());
  ui.SCCurvL->setText(QString("Curvature: %1").arg(axis->getLCurv(), 2, 'f', 2));
  ui.SCCurvR->setText(QString("Curvature: %1").arg(axis->getRCurv(), 2, 'f', 2));
  ui.SCDZoneLabel->setText(QString("DeadZone: %1").arg(axis->getDZone(), 2, 'f', 2));
}

void SCurve::setEnabled(int state)
{
  if(state == Qt::Checked){
    //std::cout<<"Enabling..."<<std::endl;
    if(!initializing) axis->changeEnabled(true);
  }else{
    //std::cout<<"Disabling..."<<std::endl;
    if(!initializing) axis->changeEnabled(false);
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
  if(!initializing) axis->changeLFactor(d);
  if(symetrical){
    ui.SCRightFactor->setValue(d);
  }
  emit changed();
}

void SCurve::on_SCRightFactor_valueChanged(double d)
{
  //std::cout<<"RightFactor = "<<d<<std::endl;
  if(!initializing) axis->changeRFactor(d);
  if(symetrical){
    ui.SCLeftFactor->setValue(d);
  }
  emit changed();
}

void SCurve::on_SCLeftCurv_valueChanged(int value)
{
  //std::cout<<"LeftCurv = "<<value<<std::endl;
  if(!initializing) axis->changeLCurv(value / 100.0);
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
  if(!initializing) axis->changeRCurv(value / 100.0);
  ui.SCCurvR->setText(QString("Curvature: %1").arg(value / 100.0, 2, 'f', 2));
  emit changed();
}

void SCurve::on_SCDeadZone_valueChanged(int value)
{
  //std::cout<<"DeadZone = "<<value<<std::endl;
  if(!initializing) axis->changeDZone(value / 101.0);
  ui.SCDZoneLabel->setText(QString("DeadZone: %1").arg(value / 101.0, 2, 'f', 2));
  emit changed();
}

void SCurve::on_SCLeftLimit_valueChanged(double d)
{
  //std::cout<<"LLimit = "<<d<<std::endl;
  if(!initializing) axis->changeLLimit(d);
  if(symetrical){
    ui.SCRightLimit->setValue(d);
  }
  emit changed();
}

void SCurve::on_SCRightLimit_valueChanged(double d)
{
  //std::cout<<"RLimit = "<<d<<std::endl;
  if(!initializing) axis->changeRLimit(d);
  if(symetrical){
    ui.SCLeftLimit->setValue(d);
  }
  emit changed();
}

void SCurve::movePoint(float new_x)
{
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
  view->movePoint(new_x);
}


void SCurve::axisChanged(AxisElem_t what)
{
  switch(what){
    case LFACTOR:
      ui.SCLeftFactor->setValue(axis->getLFactor());
      break;
    case RFACTOR:
      ui.SCRightFactor->setValue(axis->getRFactor());
      break;
    case LCURV:
      ui.SCLeftCurv->setValue(round(axis->getLCurv() * 100.0));
      ui.SCCurvL->setText(QString("Curvature: %1").arg(axis->getLCurv(), 2, 'f', 2));
      break;
    case RCURV:
      ui.SCRightCurv->setValue(round(axis->getRCurv() * 100.0));
      ui.SCCurvR->setText(QString("Curvature: %1").arg(axis->getRCurv(), 2, 'f', 2));
      break;
    case DZONE:
      ui.SCDeadZone->setValue(round(axis->getDZone() * 101.0));
      ui.SCDZoneLabel->setText(QString("DeadZone: %1").arg(axis->getDZone(), 2, 'f', 2));
      break;
    case LLIMIT:
      ui.SCLeftLimit->setValue(axis->getLLimit());
      break;
    case RLIMIT:
      ui.SCRightLimit->setValue(axis->getRLimit());
      break;
    case RELOAD:
      ui.SCLeftFactor->setValue(axis->getLFactor());
      ui.SCRightFactor->setValue(axis->getRFactor());
      ui.SCLeftCurv->setValue(round(axis->getLCurv() * 100.0));
      ui.SCRightCurv->setValue(round(axis->getRCurv() * 100.0));
      ui.SCDeadZone->setValue(round(axis->getDZone() * 101.0));
      ui.SCCurvL->setText(QString("Curvature: %1").arg(axis->getLCurv(), 2, 'f', 2));
      ui.SCCurvR->setText(QString("Curvature: %1").arg(axis->getRCurv(), 2, 'f', 2));
      ui.SCDZoneLabel->setText(QString("DeadZone: %1").arg(axis->getDZone(), 2, 'f', 2));
      ui.SCLeftLimit->setValue(axis->getLLimit());
      ui.SCRightLimit->setValue(axis->getRLimit());
      if(axis->isSymetrical()){
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

