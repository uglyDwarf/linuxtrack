#ifndef WIIMOTE_PREFS__H
#define WIIMOTE_PREFS__H

#include <QObject>
#include <QComboBox>
#include "ui_ltr.h"
#include "pref.hpp"
#include "prefs_link.h"


class WiimotePrefs : public QObject{
  Q_OBJECT
 public:
  WiimotePrefs(const Ui::LinuxtrackMainForm &ui);
  ~WiimotePrefs();
  bool Activate(const QString &ID, bool init = false);
  static bool AddAvailableDevices(QComboBox &combo);
 private:
  const Ui::LinuxtrackMainForm &gui;
  void Connect();
  bool initializing;
 private slots:
  void runIndicationChanged(int state);
  void pauseIndicationChanged(int state);
};


#endif
