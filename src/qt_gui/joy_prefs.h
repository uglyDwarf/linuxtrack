#ifndef JOY_PREFS__H
#define JOY_PREFS__H

#include <QObject>
#include <QComboBox>
#include "ui_joy_setup.h"
#include "pref.hpp"
#include "prefs_link.h"

class JoyPrefs: public QWidget{
  Q_OBJECT
 public:
  JoyPrefs(const QString &dev_id, QWidget *parent = 0);
  ~JoyPrefs();
  static bool AddAvailableDevices(QComboBox &combo);
 private:
  const QString id;
  bool Activate(const QString &ID, bool init = false);
  void Connect();
  Ui::JoySetup ui;
  bool initializing;
  static void *libhandle;
 private slots:
  void on_PitchCombo_activated(int index);
  void on_YawCombo_activated(int index);
  void on_RollCombo_activated(int index);
  void on_TXCombo_activated(int index);
  void on_TYCombo_activated(int index);
  void on_TZCombo_activated(int index);
  void on_JsButton_pressed();
  void on_EvdevButton_pressed();
  void on_PPSFreq_valueChanged(int i);
};



#endif

