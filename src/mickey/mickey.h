#ifndef MICKEY__H
#define MICKEY__H

#include <QWidget>
#include <QThread>
#include <QMutex>
#include <QTime>
#include <QTimer>
#include <QSettings>
#include <QDialog>
#include "ui_mickey.h"
#include "ui_calibration.h"
#include "ui_chsettings.h"
#include "keyb.h"
#include "sn4_com.h"

class MickeyApplyDialog: public QWidget
{
 Q_OBJECT
 public:
  MickeyApplyDialog(QWidget *parent = 0);
  void trySettings();
 private slots:
  void on_RevertButton_pressed();
  void on_KeepButton_pressed();
  void timeout();
 signals:
  void revert();
  void keep();
 private:
  Ui::AcceptSettings ui;
  QTimer timer;
  int cntr;
};


class MickeyCalibration : public QWidget
{
 Q_OBJECT
 public:
  MickeyCalibration(QWidget *parent = 0);
  void setText(const QString &s){ui.CalibrationText->setText(s);};
 private:
  Ui::Calibration ui;
 signals:
  void nextClicked();
  void cancelClicked();
};

typedef enum {CENTER, CALIBRATE} cal_state_t;
typedef enum {TRACKING, STANDBY, CALIBRATING} state_t;

class MickeyUinput
{
 public:
  MickeyUinput();
  ~MickeyUinput();
  bool init();
  void mouseClick(sn4_btn_event_t ev);
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
 public slots:
  void on_key_pressed();
 signals:
  void clicked();
 private:
  void processClick(sn4_btn_event_t ev);
  int fifo;
  bool finish;
  const Mickey &parent;
  int fakeBtn;
};

class MickeysAxis : public QObject
{
 Q_OBJECT
 public:
  MickeysAxis(const QString &id);
  ~MickeysAxis();
  int updateAxis(float val, int elapsed);
  int getDeadZone(){return deadZone;};
  int getSensitivity(){return sensitivity;};
  void changeDeadZone(int dz);
  void changeSensitivity(int sens);
  void startCalibration();
  void finishCalibration();
  void cancelCalibration();
  float getSpeed(int sens);
 private:
  float processValue(float val, int elapsed);
  int deadZone;
  int sensitivity;
  float accumulator;
  const QString identificator;
  QSettings settings;
  bool calibrating;
  float maxVal, minVal, prevMaxVal;
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
  shortcut *onOffSwitch;
  shortcut lbtnSwitch;
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
  QDialog adg;
  MickeyApplyDialog aplDlg;
  QTime initTimer;
  QTime updateElapsed;
  bool recenterFlag;
 private slots:
  void on_CalibrateButton_pressed();
  void on_ApplyButton_pressed();
  void onOffSwitch_activated();
  void updateTimer_activated();
  void on_DZSlider_valueChanged(int value);
  void on_SensSlider_valueChanged(int value);
  void threadClicked();
  void calibrationCancelled();
  void keepSettings();
  void revertSettings();
};

#endif