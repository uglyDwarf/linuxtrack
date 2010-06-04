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
  symetrical = true;
  ui.setupUi(this);
  ui.SCTitle->setText(axis_name);
  ui.SCLeftLabel->setText(left_label);
  ui.SCRightLabel->setText(right_label);
  QObject::connect(axis, SIGNAL(axisChanged(AxisElem_t)),
                    this, SLOT(axisChanged(AxisElem_t)));
  
  first = false;
  view = new SCView(axis, ui.SCView);
  QObject::connect(this, SIGNAL(changed()), view, SLOT(update()));
  axisChanged(RELOAD);
}

SCurve::~SCurve()
{
  delete view;
}

void SCurve::setup_gui()
{
  if(axis->isSymetrical()){
    ui.SCSymetrical->setCheckState(Qt::Checked);
  }else{
    ui.SCSymetrical->setCheckState(Qt::Unchecked);
  }
  ui.SCLeftFactor->setValue(axis->getLFactor());
  ui.SCLeftCurv->setValue(axis->getLCurv() * 100);
  ui.SCRightFactor->setValue(axis->getRFactor());
  ui.SCRightCurv->setValue(axis->getRCurv() * 100);
  ui.SCDeadZone->setValue(axis->getDZone() * 101);
  ui.SCInputLimits->setValue(axis->getLimits());
}

void SCurve::setEnabled(int state)
{
  if(state == Qt::Checked){
    //std::cout<<"Enabling..."<<std::endl;
    axis->changeEnabled(true);
  }else{
    //std::cout<<"Disabling..."<<std::endl;
    axis->changeEnabled(false);
  }
}

void SCurve::on_SCSymetrical_stateChanged(int state)
{
  switch(state){
    case Qt::Checked:
      symetrical = true;
      ui.SCRightFactor->setDisabled(true);
      ui.SCRightFactor->setValue(ui.SCLeftFactor->value());
      ui.SCRightCurv->setDisabled(true);
      ui.SCRightCurv->setValue(ui.SCLeftCurv->value());
      break;
    case Qt::Unchecked:
      symetrical = false;
      ui.SCRightFactor->setDisabled(false);
      ui.SCRightCurv->setDisabled(false);
      break;
    default:
      break;
  }
}

void SCurve::on_SCLeftFactor_valueChanged(double d)
{
  //std::cout<<"LeftFactor = "<<d<<std::endl;
  axis->changeLFactor(d);
  if(symetrical){
    ui.SCRightFactor->setValue(d);
  }
  emit changed();
}

void SCurve::on_SCRightFactor_valueChanged(double d)
{
  //std::cout<<"RightFactor = "<<d<<std::endl;
  axis->changeRFactor(d);
  if(symetrical){
    ui.SCLeftFactor->setValue(d);
  }
  emit changed();
}

void SCurve::on_SCLeftCurv_valueChanged(int value)
{
  //std::cout<<"LeftCurv = "<<value<<std::endl;
  axis->changeLCurv(value / 100.0);
  if(symetrical){
    ui.SCRightCurv->setValue(value);
  }else{
    emit changed();
  }
}

void SCurve::on_SCRightCurv_valueChanged(int value)
{
  //std::cout<<"RightCurv = "<<value<<std::endl;
  axis->changeRCurv(value / 100.0);
  emit changed();
}

void SCurve::on_SCDeadZone_valueChanged(int value)
{
  //std::cout<<"DeadZone = "<<value<<std::endl;
  axis->changeDZone(value / 101.0);
  emit changed();
}

void SCurve::on_SCInputLimits_valueChanged(double d)
{
  //std::cout<<"Limits = "<<d<<std::endl;
  axis->changeLimits(d);
  emit changed();
}

void SCurve::movePoint(float new_x)
{
  float val = new_x / axis->getLimits();
  if(val > 1.0f){
    val = 1.0f;
  }
  if(val < -1.0f){
    val = -1.0f;
  }
  view->movePoint(val);
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
      break;
    case RCURV:
      ui.SCRightCurv->setValue(round(axis->getRCurv() * 100.0));
      break;
    case DZONE:
      ui.SCDeadZone->setValue(round(axis->getDZone() * 101.0));
      break;
    case LIMITS:
      ui.SCInputLimits->setValue(axis->getLimits());
      break;
    case RELOAD:
      ui.SCLeftFactor->setValue(axis->getLFactor());
      ui.SCRightFactor->setValue(axis->getRFactor());
      ui.SCLeftCurv->setValue(round(axis->getLCurv() * 100.0));
      ui.SCRightCurv->setValue(round(axis->getRCurv() * 100.0));
      ui.SCDeadZone->setValue(round(axis->getDZone() * 101.0));
      ui.SCInputLimits->setValue(axis->getLimits());
      if(axis->isSymetrical()){
	ui.SCSymetrical->setCheckState(Qt::Checked);
      }else{
	ui.SCSymetrical->setCheckState(Qt::Unchecked);
      }
      break;
    default:
      break;
  }
  emit changed();
}

