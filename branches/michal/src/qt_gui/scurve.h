#ifndef SCURVE__H
#define SCURVE__H

#include <QWidget>
#include <QString>

#include "ui_scurve.h"
#include "scview.h"
#include "spline.h"

class SCurve : public QWidget{
  Q_OBJECT
 public:
  SCurve(QString axis_name, QString title, QString left, QString right, QWidget *parent = 0);
  ~SCurve();
 signals:
  void changed();
 private slots:
  void on_SCClose_pressed();
  void on_SCSymetrical_stateChanged(int state);
  void on_SCLeftFactor_valueChanged(double d);
  void on_SCRightFactor_valueChanged(double d);
  void on_SCLeftCurv_valueChanged(int value);
  void on_SCRightCurv_valueChanged(int value);
  void on_SCDeadZone_valueChanged(int value);
 private:
  Ui::SCurveForm ui;
  bool symetrical;
  axis_def axis;
  SCView *view;
};

#endif