#ifndef SCURVE__H
#define SCURVE__H

#include <QWidget>
#include <QString>

#include "ui_scurve.h"
#include "scview.h"
#include <axis.h>

class SCurve : public QWidget{
  Q_OBJECT
 public:
  SCurve(axis_t a, QString axis_name, QString left, QString right, QWidget *parent = 0);
  ~SCurve();
  //void movePoint(float new_x);
 signals:
  void changed();
  //void movePoint(float new_x, float new_y);
  void symetryChanged(bool symetrical);
 private slots:
  void on_SCSymetrical_stateChanged(int state);
  void on_SCFactor_valueChanged(int value);
  void on_SCLeftCurv_valueChanged(int value);
  void on_SCRightCurv_valueChanged(int value);
  void on_SCFilterSlider_valueChanged(int value);
  void on_SCDeadZone_valueChanged(int value);
  void on_SCLeftLimit_valueChanged(double d);
  void on_SCRightLimit_valueChanged(double d);
  void setEnabled(int state);
  void axisChanged(int a, int elem);
  void initAxes();
 private:
  void setup_gui();
  axis_t axis;
  Ui::SCurveForm ui;
  bool symetrical;
  SCView *view;
  bool first;
  bool initializing;
  
  void setDeadzone(float val, bool signal = false);
  void setFilter(float val, bool signal = false);

};

#endif
