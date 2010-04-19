#include <iostream>
#include "scurve.h"
#include "pref_global.h"
#include "ltr_gui_prefs.h"

SCurve::SCurve(QString prefix, QString axis_name, QString left_label, QString right_label, QWidget *parent)
  : QWidget(parent), prefPrefix(prefix)
{
  symetrical = true;
  ui.setupUi(this);
  ui.SCTitle->setText(axis_name);
  ui.SCLeftLabel->setText(left_label);
  ui.SCRightLabel->setText(right_label);
  
  get_axis(prefix.toAscii().data(), &axis, NULL);
  setup_gui();
  view = new SCView(axis, ui.SCView);
  QObject::connect(this, SIGNAL(changed()), view, SLOT(update()));
}

SCurve::~SCurve()
{
  delete view;
  close_axis(&axis);
}

void SCurve::setup_gui()
{
  if(is_symetrical(axis)){
    ui.SCSymetrical->setCheckState(Qt::Checked);
  }else{
    ui.SCSymetrical->setCheckState(Qt::Unchecked);
  }
  ui.SCLeftFactor->setValue(get_lmult(axis));
  ui.SCLeftCurv->setValue(get_lcurv(axis) * 100);
  ui.SCRightFactor->setValue(get_rmult(axis));
  ui.SCRightCurv->setValue(get_rcurv(axis) * 100);
  ui.SCDeadZone->setValue(get_deadzone(axis) * 101.0);
  ui.SCInputLimits->setValue(get_limits(axis));
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
  std::cout<<"LeftFactor = "<<d<<std::endl;
  PREF.setKeyVal("Default", prefPrefix + "-left-multiplier", d);
  set_lmult(axis, d);
  if(symetrical){
    ui.SCRightFactor->setValue(d);
  }else{
    emit changed();
  }
}

void SCurve::on_SCRightFactor_valueChanged(double d)
{
  std::cout<<"RightFactor = "<<d<<std::endl;
  PREF.setKeyVal("Default", prefPrefix + "-right-multiplier", d);
  set_rmult(axis, d);
  emit changed();
}

void SCurve::on_SCLeftCurv_valueChanged(int value)
{
  std::cout<<"LeftCurv = "<<value<<std::endl;
  PREF.setKeyVal("Default", prefPrefix + "-left-curvature", value / 100.0);
  set_lcurv(axis, value / 100.0);
  if(symetrical){
    ui.SCRightCurv->setValue(value);
  }else{
    emit changed();
  }
}

void SCurve::on_SCRightCurv_valueChanged(int value)
{
  std::cout<<"RightCurv = "<<value<<std::endl;
  PREF.setKeyVal("Default", prefPrefix + "-right-curvature", value / 100.0);
  set_rcurv(axis, value / 100.0);
  emit changed();
}

void SCurve::on_SCDeadZone_valueChanged(int value)
{
  std::cout<<"DeadZone = "<<value<<std::endl;
  PREF.setKeyVal("Default", prefPrefix + "-deadzone", value / 101.0);
  set_deadzone(axis, value / 101.0);
  emit changed();
}

void SCurve::on_SCInputLimits_valueChanged(double d)
{
  std::cout<<"Limits = "<<d<<std::endl;
  PREF.setKeyVal("Default", prefPrefix + "-limits", d);
  set_limits(axis, d);
  emit changed();
}

void SCurve::movePoint(float new_x)
{
  float val = new_x / get_limits(axis);
  if(val > 1.0f){
    val = 1.0f;
  }
  if(val < -1.0f){
    val = -1.0f;
  }
  view->movePoint(val);
}

