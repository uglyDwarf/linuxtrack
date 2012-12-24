#ifndef MICKEY_TRANSFORM__H
#define MICKEY_TRANSFORM__H

#include <QWidget>
#include <QBoxLayout>
#include <QSettings>
#include <QPixmap>

#include "ui_pref.h"

class MickeyCurveShow : public QWidget
{
 Q_OBJECT
 public:
  MickeyCurveShow(QWidget *parent = 0);
  void updatePixmap(QPointF points[], int pointsCount);
 private:
  QPixmap *img;
  virtual void paintEvent(QPaintEvent * /* event */);
  virtual void resizeEvent(QResizeEvent * event){emit resized(); QWidget::resizeEvent(event);};
 signals:
  void resized();
};

class MickeysAxis : public QWidget
{
 Q_OBJECT
 public:
  MickeysAxis(QBoxLayout *parent = 0);
  ~MickeysAxis();
  void step(float valX, float valY, int elapsed, float &accX, float &accY);
 private:
  Ui::AxisPrefs ui;
  float response(float mag);
  int getDeadZone(){return deadZone;};
  int getSensitivity(){return sensitivity;};
  void changeDeadZone(int dz);
  void changeSensitivity(int sens);
  float getSpeed(int sens);
  int sensitivity, deadZone, curv;
  bool stepOnly;
  QSettings settings;
  void updatePixmap();
  MickeyCurveShow *curveShow;
// public slots:
//  void redraw(){update();};
 private slots:
  void on_SensSlider_valueChanged(int val){sensitivity = val;};
  void on_DZSlider_valueChanged(int val){deadZone = val; updatePixmap();};
  void on_CurveSlider_valueChanged(int val){curv = val; updatePixmap();};
  void on_StepOnly_stateChanged(int state);
  void curveShow_resized(){updatePixmap();};
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
  float maxValX, minValX, prevMaxValX, maxValY, minValY, prevMaxValY;
  MickeysAxis axis;
};



#endif
