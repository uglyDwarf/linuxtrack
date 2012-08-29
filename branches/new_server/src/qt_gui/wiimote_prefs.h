#ifndef WIIMOTE_PREFS__H
#define WIIMOTE_PREFS__H

#include <QObject>
#include <QComboBox>
#include "ui_wii_setup.h"
#include "pref.hpp"
#include "prefs_link.h"


class WiimotePrefs : public QWidget{
  Q_OBJECT
 public:
  WiimotePrefs(QWidget *parent = 0);
  ~WiimotePrefs();
  bool Activate(const QString &ID, bool init = false);
  static bool AddAvailableDevices(QComboBox &combo);
 private:
  void Connect();
  Ui::WiiSetupForm ui;
  bool initializing;
 private slots:
  void runIndicationChanged(int state);
  void pauseIndicationChanged(int state);
};


#endif
