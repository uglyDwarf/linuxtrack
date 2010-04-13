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
}

void SCurve::setup_gui()
{
  if((axis.l_factor == axis.r_factor) && 
     (axis.curves.l_curvature == axis.curves.r_curvature)){
    ui.SCSymetrical->setCheckState(Qt::Checked);
  }else{
    ui.SCSymetrical->setCheckState(Qt::Unchecked);
  }
  ui.SCLeftFactor->setValue(axis.l_factor);
  ui.SCLeftCurv->setValue(axis.curves.l_curvature * 100);
  ui.SCRightFactor->setValue(axis.r_factor);
  ui.SCRightCurv->setValue(axis.curves.r_curvature * 100);
  ui.SCDeadZone->setValue(axis.curves.dead_zone * 101.0);
  ui.SCInputLimits->setValue(axis.limits);
  
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
  axis.l_factor = d;
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
  axis.r_factor = d;
  emit changed();
}

void SCurve::on_SCLeftCurv_valueChanged(int value)
{
  std::cout<<"LeftCurv = "<<value<<std::endl;
  PREF.setKeyVal("Default", prefPrefix + "-left-curvature", value / 100.0);
  axis.curves.l_curvature = value / 100.0;
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
  axis.curves.r_curvature = value / 100.0;
  emit changed();
}

void SCurve::on_SCDeadZone_valueChanged(int value)
{
  std::cout<<"DeadZone = "<<value<<std::endl;
  PREF.setKeyVal("Default", prefPrefix + "-deadzone", value / 101.0);
  axis.curves.dead_zone = value / 101.0; //For DZ == 1.0 strange things happen...
  emit changed();
}

void SCurve::on_SCInputLimits_valueChanged(double d)
{
  std::cout<<"Limits = "<<d<<std::endl;
  PREF.setKeyVal("Default", prefPrefix + "-limits", d);
  axis.limits = d;
}
