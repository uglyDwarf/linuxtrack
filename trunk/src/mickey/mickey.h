#ifndef MICKEY__H
#define MICKEY__H

#include <QxtGlobalShortcut>
#include <QWidget>
#include <QThread>
#include <QMutex>
#include <QTimer>
#include <QSettings>
#include <QDialog>
#include "ui_mickey.h"
#include "ui_calibration.h"



class MickeyCalibration : public QWidget
{
 Q_OBJECT
 public:
  MickeyCalibration(QWidget *parent = 0);
  void setText(const QString &s){ui.CalibrationText->setText(s);};
 private:
  Ui::Calibration ui;
};

typedef enum {CENTER, CALIBRATE} cal_state_t;
typedef enum {TRACKING, STANDBY, CALIBRATING} state_t;

class MickeyUinput
{
 public:
  MickeyUinput();
  ~MickeyUinput();
  bool init();
  void mouseClick(int btns);
  void mouseMove(int dx, int dy);
 private:
  int fd;
  static QMutex mutex;
};

class Mickey;

//Thread processing sidechannel data (SN4 clicks)
class MickeyThread : public QThread
{
 Q_OBJECT
 public:
  MickeyThread(Mickey *p = 0);
  void run();
  void setFinish(){finish = true;};
 private slots:
  void on_key_pressed();
 signals:
  void clicked();
 private:
  int fifo;
  bool finish;
  const Mickey &parent;
  QxtGlobalShortcut lbtnSwitch;
  int fakeBtn;
};

class MickeysAxis : public QObject
{
 Q_OBJECT
 public:
  MickeysAxis(const QString &id);
  ~MickeysAxis();
  int updateAxis(float val);
  int getDeadZone(){return deadZone;};
  int getSensitivity(){return sensitivity;};
  void changeDeadZone(int dz);
  void changeSensitivity(int sens);
  void startCalibration();
  void finishCalibration();
 private:
  float processValue(float val);
  int deadZone;
  int sensitivity;
  float accumulator;
  const QString identificator;
  QSettings settings;
  bool calibrating;
  float maxVal, minVal;
};

//typedef enum {PREP} cal_state_t;

class Mickey : public QWidget
{
 Q_OBJECT
 public: 
  Mickey(QWidget *parent = 0);
  ~Mickey();
  state_t getState() const{return state;};
 private:
  Ui::Mickey ui;
  QxtGlobalShortcut *onOffSwitch;
  QTimer updateTimer;
  QTimer testTimer;
  MickeysAxis x, y;
  MickeyThread btnThread;
  state_t state;
  cal_state_t calState;
  void pause();
  void wakeup();
  void startCalibration();
  void changeState(state_t state);
  QDialog cdg;
  MickeyCalibration calDlg;
 private slots:
  void on_CalibrateButton_pressed();
  void on_ApplyButton_pressed();
  void on_onOffSwitch_activated();
  void on_updateTimer_activated();
  void on_DZSlider_valueChanged(int value);
  void on_SensSlider_valueChanged(int value);
  void on_thread_clicked();
};

#endif