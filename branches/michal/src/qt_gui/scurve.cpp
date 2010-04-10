#include <iostream>
#include "scurve.h"


SCurve::SCurve(QString axis_name, QString title, QString left_label, QString right_label, QWidget *parent)
  : QWidget(parent)
{
  symetrical = true;
  ui.setupUi(this);
  ui.SCTitle->setText(title);
  ui.SCLeftLabel->setText(left_label);
  ui.SCRightLabel->setText(right_label);
  
  //for now only!!!
  axis.curves.dead_zone = 0.0;
  axis.curves.l_curvature = axis.curves.r_curvature = 0.5;
  axis.l_factor = axis.r_factor = 1.0f;
  view = new SCView(axis, ui.SCView);
  QObject::connect(this, SIGNAL(changed()), view, SLOT(update()));
}

SCurve::~SCurve()
{
  delete view;
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
  axis.r_factor = d;
  emit changed();
}

void SCurve::on_SCLeftCurv_valueChanged(int value)
{
  std::cout<<"LeftCurv = "<<value<<std::endl;
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
  axis.curves.r_curvature = value / 100.0;
  emit changed();
}

void SCurve::on_SCDeadZone_valueChanged(int value)
{
  std::cout<<"DeadZone = "<<value<<std::endl;
  axis.curves.dead_zone = value / 101.0; //For DZ == 1.0 strange things happen...
  emit changed();
}

void SCurve::on_SCClose_pressed()
{
  close();
}
