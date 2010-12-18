#ifndef SCURVE__H
#define SCURVE__H

#include <QWidget>
#include <QString>

#include "ui_scurve.h"
#include "scview.h"
#include "ltr_axis.h"

class SCurve : public QWidget{
  Q_OBJECT
 public:
  SCurve(LtrAxis *a, QString axis_name, QString left, QString right, QWidget *parent = 0);
  ~SCurve();
  void movePoint(float new_x);
 signals:
  void changed();
  //void movePoint(float new_x, float new_y);
  void symetryChanged(bool symetrical);
 private slots:
  void on_SCSymetrical_stateChanged(int state);
  void on_SCLeftFactor_valueChanged(double d);
  void on_SCRightFactor_valueChanged(double d);
  void on_SCLeftCurv_valueChanged(int value);
  void on_SCRightCurv_valueChanged(int value);
  void on_SCDeadZone_valueChanged(int value);
  void on_SCInputLimits_valueChanged(double d);
  void setEnabled(int state);
  void axisChanged(AxisElem_t what);
 private:
  void setup_gui();
  LtrAxis *axis;
  Ui::SCurveForm ui;
  bool symetrical;
  SCView *view;
  bool first;
  bool initializing;
};

#endif