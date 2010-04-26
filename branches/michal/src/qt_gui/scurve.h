#ifndef SCURVE__H
#define SCURVE__H

#include <QWidget>
#include <QString>

#include "ui_scurve.h"
#include "scview.h"
#include "axis.h"

class SCurve : public QWidget{
  Q_OBJECT
 public:
  SCurve(QString prefix, QString axis_name, QString left, QString right, QWidget *parent = 0);
  ~SCurve();
  void movePoint(float new_x);
  void setSlaves(QCheckBox *en, QDoubleSpinBox *l_spin, QDoubleSpinBox *r_spin);
  void reinit();
 signals:
  void changed();
  float movePoint(float new_x, float new_y);
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
 private:
  void setup_gui();
  Ui::SCurveForm ui;
  bool symetrical;
  struct axis_def *axis;
  QString prefPrefix;
  SCView *view;
  bool first;
};

#endif