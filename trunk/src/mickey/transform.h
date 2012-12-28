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
  void updatePixmap(QPointF curPoints[], QPointF newPoints[], int pointCount);
 private:
  QPixmap *img;
  virtual void paintEvent(QPaintEvent * /* event */);
  virtual void resizeEvent(QResizeEvent * event){emit resized(); QWidget::resizeEvent(event);};
 signals:
  void resized();
};

typedef struct {
  int sensitivity, deadZone, curv;
  bool stepOnly;
} setup_t;

class MickeysAxis : public QWidget
{
 Q_OBJECT
 public:
  MickeysAxis(QSettings &s, QBoxLayout *parent = 0);
  ~MickeysAxis();
  void step(float valX, float valY, int elapsed, float &accX, float &accY);
  void applySettings();
  void revertSettings();
 private:
  float response(float mag, setup_t *s = NULL);
  int getDeadZone(){return setup.deadZone;};
  int getSensitivity(){return setup.sensitivity;};
  //void changeDeadZone(int dz);
  //void changeSensitivity(int sens);
  float getSpeed(int sens);
  void updatePixmap();
  
  Ui::AxisPrefs ui;
  
  setup_t oldSetup, newSetup, setup;
  QSettings &settings;
  MickeyCurveShow *curveShow;
// public slots:
//  void redraw(){update();};
 private slots:
  void on_SensSlider_valueChanged(int val){newSetup.sensitivity = val;emit newSettings();};
  void on_DZSlider_valueChanged(int val)
    {newSetup.deadZone = val; updatePixmap();emit newSettings();};
  void on_CurveSlider_valueChanged(int val)
    {newSetup.curv = val; updatePixmap();emit newSettings();};
  void on_StepOnly_stateChanged(int state);
  void curveShow_resized(){updatePixmap();};
 signals:
  void newSettings();
};


class MickeyTransform : public QObject
{
 Q_OBJECT
 public:
  MickeyTransform(QSettings &s, QBoxLayout *parent = 0);
  ~MickeyTransform();
  void update(float valX, float valY, int elapsed, int &x, int &y);
  void startCalibration();
  void finishCalibration();
  void cancelCalibration();
  void applySettings(){axis.applySettings();};
  void revertSettings(){axis.revertSettings();};
 signals:
  void newSettings();
 private:
  float accX, accY;
  QSettings &settings;
  bool calibrating;
  float maxValX, minValX, prevMaxValX, maxValY, minValY, prevMaxValY;
  MickeysAxis axis;
};



#endif
