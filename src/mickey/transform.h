#ifndef MICKEY_TRANSFORM__H
#define MICKEY_TRANSFORM__H

#include <QWidget>
#include <QBoxLayout>
#include <QSettings>

#include "ui_pref.h"

class MickeysAxis : public QWidget
{
 Q_OBJECT
 public:
  MickeysAxis(QBoxLayout *parent = 0);
  ~MickeysAxis();
  void step(float valX, float valY, int elapsed, float &accX, float &accY);
 private:
  Ui::AxisPrefs ui;
  int getDeadZone(){return deadZone;};
  int getSensitivity(){return sensitivity;};
  void changeDeadZone(int dz);
  void changeSensitivity(int sens);
  float getSpeed(int sens);
  int sensitivity, deadZone, curv;
  bool stepOnly;
  QSettings settings;
 private slots:
  void on_SensSlider_valueChanged(int val){sensitivity = val;};
  void on_DZSlider_valueChanged(int val){deadZone = val;};
  void on_CurveSlider_valueChanged(int val){curv = val;};
  void on_StepOnly_stateChanged(int state){stepOnly = (state == Qt::Checked) ? true : false;};
};


class MickeyTransform
{
 public:
  MickeyTransform(QBoxLayout *parent = 0);
  ~MickeyTransform();
  void update(float valX, float valY, int elapsed, int &x, int &y);
  void startCalibration();
  void finishCalibration();
  void cancelCalibration();
 private:
  float accX, accY;
  QSettings settings;
  bool calibrating;
  float maxVal, minVal, prevMaxVal;
  MickeysAxis axis;
};



#endif
