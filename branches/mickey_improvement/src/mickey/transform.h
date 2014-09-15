#ifndef MICKEY_TRANSFORM__H
#define MICKEY_TRANSFORM__H

#include <QWidget>
#include <QBoxLayout>
#include <QSettings>
#include <QPixmap>

#include "mickey.h"

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
  int sensitivity, deadzone, curvature;
  bool stepOnly;
} setup_t;

class MickeysAxis : public QObject
{
 Q_OBJECT
 public:
  MickeysAxis();
  ~MickeysAxis();
  void step(float valX, float valY, int elapsed, float &accX, float &accY);
  void applySettings();
  void revertSettings();
  void keepSettings();
 private:
  int sensitivity;
  float response(float mag, setup_t *s = NULL);
  float getSpeed(int sens);
  void updatePixmap();
  
  setup_t oldSetup, newSetup, setup;
  MickeyCurveShow *curveShow;
// public slots:
//  void redraw(){update();};
 private slots:
  void axisChanged();
  void curveShow_resized(){updatePixmap();};
 signals:
  void newSettings();
};


class MickeyTransform : public QObject
{
 Q_OBJECT
 public:
  MickeyTransform();
  ~MickeyTransform();
  void update(float valX, float valY, bool relative, int elapsed, float &x, float &y);
  void startCalibration();
  void finishCalibration();
  void cancelCalibration();
  void applySettings();
  void revertSettings();
  void keepSettings();
 private:
  float accX, accY;
  bool calibrating;
  float maxValX, minValX, maxValY, minValY, prevMaxValX, prevMaxValY;
  float currMaxValX, currMaxValY;
  MickeysAxis axis;
};



#endif
