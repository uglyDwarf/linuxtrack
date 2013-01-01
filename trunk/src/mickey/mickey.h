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
  void recenter(){/* for now */};
  void calibrate(){changeState(CALIBRATING);};
  bool setShortcut(QKeySequence seq){return onOffSwitch->setShortcut(seq);};
 private:
  shortcut *onOffSwitch;
  QTimer updateTimer;
  MickeyTransform *trans;
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
  void onOffSwitch_activated();
  void updateTimer_activated();
  void threadClicked();
  void calibrationCancelled();
  //void keepSettings();
  void revertSettings();
  //void newSettings();
};

#define GUI MickeyGUI::getInstance()

class MickeyGUI : public QWidget
{
  Q_OBJECT
 public:
  static MickeyGUI &getInstance();
  //Axis interface
  int getSensitivity(){return sensitivity;};
  int getDeadzone(){return deadzone;};
  int getCurvature(){return curvature;};
  bool getStepOnly(){return stepOnly;};
  void setSensitivity(int value){sensitivity = value; ui.SensSlider->setValue(sensitivity);};
  void setDeadzone(int value){deadzone = value; ui.DZSlider->setValue(deadzone);};
  void setCurvature(int value){curvature = value; ui.CurveSlider->setValue(curvature);};
  void setStepOnly(bool value);
  QBoxLayout *getAxisViewLayout(){return ui.PrefLayout;};
  //Transform interface
  void getMaxVal(float &x, float &y){x = maxValX; y = maxValY;};
  void setMaxVal(float x, float y){maxValX = x; maxValY = y;};
  void setStatusLabel(const QString &text){ui.StatusLabel->setText(text);};
 private:
  void init(){mickey = new Mickey();};
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
  void getShortcut();
  
 private slots:
  void on_SensSlider_valueChanged(int val){sensitivity = val; emit axisChanged(); ui.ApplyButton->setEnabled(true);};
  void on_DZSlider_valueChanged(int val){deadzone = val; emit axisChanged(); ui.ApplyButton->setEnabled(true);};
  void on_CurveSlider_valueChanged(int val){curvature = val; emit axisChanged(); ui.ApplyButton->setEnabled(true);};
  void on_StepOnly_stateChanged(int state);
  void on_ApplyButton_pressed(){ui.ApplyButton->setEnabled(false); mickey->applySettings();};
  void on_CalibrateButton_pressed(){if(mickey != NULL){mickey->calibrate();}};
  void on_RecenterButton_pressed(){if(mickey != NULL){mickey->recenter();}};
  void on_HelpButton_pressed(){/* for now */};
  void on_ModifierCombo_currentIndexChanged(const QString &text){(void) text; getShortcut();};
  void on_KeyCombo_currentIndexChanged(const QString &text){(void) text; getShortcut();};
 signals:
  void axisChanged();
};


#endif