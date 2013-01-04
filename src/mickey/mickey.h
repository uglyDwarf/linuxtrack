#ifndef MICKEY__H
#define MICKEY__H

#include <QWidget>
#include <QThread>
#include <QMutex>
#include <QTime>
#include <QTimer>
#include <QSettings>
#include <QDialog>
#include <QCloseEvent>
#include <QVBoxLayout>
#include "ui_mickey.h"
#include "ui_calibration.h"
#include "ui_chsettings.h"
#include "linuxtrack.h"
#include "keyb.h"
#include "sn4_com.h"
#include "help_view.h"

/*
class MickeyDialog: public QDialog
{
 Q_OBJECT
 public:
  MickeyDialog(){};
 signals:
  void closing();
 protected:
  virtual void closeEvent(QCloseEvent *event){emit closing(); QDialog::closeEvent(event);};
};
*/

class MickeyApplyDialog: public QDialog
{
 Q_OBJECT
 public:
  MickeyApplyDialog();
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
  virtual void closeEvent(QCloseEvent *event);
};


class MickeyCalibration : public QDialog
{
 Q_OBJECT
 public:
  MickeyCalibration();
  void setText(const QString &s){ui.CalibrationText->setText(s);};
  void calibrate();
  void recenter();
 private:
  Ui::Calibration ui;
  QTimer timer;
  int cntr;
  enum {CENTER, CALIBRATE, CENTER_ONLY} calState;
  virtual void closeEvent(QCloseEvent *event);
 private slots:
  void timeout();
  void cancelPressed();
 signals:
  void recenterNow(bool leave);
  void startCalibration();
  void finishCalibration();
  void cancelCalibration(bool calStarted);
};

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


//typedef enum {PREP} cal_state_t;
class MickeyTransform;

class Mickey : public QObject
{
 Q_OBJECT
 public: 
  Mickey();
  ~Mickey();
  state_t getState() const{return state;};
  void applySettings();
  void recenter();
  void calibrate();
  bool setShortcut(QKeySequence seq);
 private:
  shortcut *onOffSwitch;
  QTimer updateTimer;
  MickeyTransform *trans;
  MickeyThread btnThread;
  state_t state;
  void pause();
  void wakeup();
  void changeState(state_t state);
  MickeyCalibration calDlg;
  MickeyApplyDialog aplDlg;
  QTime initTimer;
  QTime updateElapsed;
  bool recenterFlag;
 private slots:
  void onOffSwitch_activated();
  void updateTimer_activated();
  void revertSettings();
  void keepSettings();
  void recenterNow(bool leave){ltr_recenter(); if(leave){changeState(TRACKING);}};
  void startCalibration();
  void finishCalibration();
  void cancelCalibration(bool calStarted);
};

#define GUI MickeyGUI::getInstance()

class MickeyGUI : public QWidget
{
  Q_OBJECT
 public:
  static MickeyGUI &getInstance();
  static void deleteInstance();
  //Axis interface
  int getSensitivity(){return sensitivity;};
  int getDeadzone(){return deadzone;};
  int getCurvature(){return curvature;};
  bool getStepOnly(){return stepOnly;};
  void setSensitivity(int value){changed = true; sensitivity = value; ui.SensSlider->setValue(sensitivity);};
  void setDeadzone(int value){changed = true; deadzone = value; ui.DZSlider->setValue(deadzone);};
  void setCurvature(int value){changed = true; curvature = value; ui.CurveSlider->setValue(curvature);};
  void setStepOnly(bool value);
  QBoxLayout *getAxisViewLayout(){return ui.PrefLayout;};
  //Transform interface
  void getMaxVal(float &x, float &y){x = maxValX; y = maxValY;};
  void setMaxVal(float x, float y){changed = true; maxValX = x; maxValY = y;};
  void setStatusLabel(const QString &text){ui.StatusLabel->setText(text);};
  
  int getCntrDelay(){return cntrDelay;};
  int getCalDelay(){return calDelay;};
 public slots:
  void show();
 private:
  void init();
  static MickeyGUI *instance;
  MickeyGUI(QWidget *parent = 0);
  ~MickeyGUI();
  void readPrefs();
  void storePrefs();
  Mickey *mickey;
  Ui::Mickey ui;
  QSettings settings;
  int sensitivity, deadzone, curvature;
  bool stepOnly;
  float maxValX, maxValY; 
  int calDelay, cntrDelay;
  bool getShortcut();
  virtual void closeEvent(QCloseEvent *event);
  bool changed;
  bool welcome;
  int newsSerial;
  int modifierIndex, hotkeyIndex, hotkeySet;
 private slots:
  void on_SensSlider_valueChanged(int val)
    {sensitivity = val; emit axisChanged(); ui.ApplyButton->setEnabled(true);};
  void on_DZSlider_valueChanged(int val)
    {deadzone = val; emit axisChanged(); ui.ApplyButton->setEnabled(true);};
  void on_CurveSlider_valueChanged(int val)
    {curvature = val; emit axisChanged(); ui.ApplyButton->setEnabled(true);};
  void on_StepOnly_stateChanged(int state);
  void on_ApplyButton_pressed()
    {changed = true; ui.ApplyButton->setEnabled(false); mickey->applySettings();};
  void on_CalibrateButton_pressed()
    {if(mickey != NULL){changed = true; mickey->calibrate();}};
  void on_RecenterButton_pressed()
    {if(mickey != NULL){mickey->recenter();}};
  void on_HelpButton_pressed()
    {HelpViewer::ShowWindow();};
  void on_ModifierCombo_currentIndexChanged(const QString &text)
    {(void) text; hotkeySet = getShortcut();};
  void on_KeyCombo_currentIndexChanged(const QString &text)
    {(void) text; hotkeySet = getShortcut();};
  void on_CalibrationTimeout_valueChanged(int val)
    {changed = true; calDelay = val;};
  void on_CenterTimeout_valueChanged(int val)
    {changed = true; cntrDelay = val;};
  void on_MickeyTabs_currentChanged(int index);
 signals:
  void axisChanged();
};


#endif