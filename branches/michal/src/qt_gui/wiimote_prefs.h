#ifndef WIIMOTE_PREFS__H
#define WIIMOTE_PREFS__H

#include <QObject>
#include <QComboBox>
#include "ui_ltr.h"
#include "pref_int.h"
#include "prefs_link.h"


class WiimotePrefs : public QObject{
  Q_OBJECT
 public:
  WiimotePrefs(const Ui::LinuxtrackMainForm &ui);
  ~WiimotePrefs();
  void Activate(const QString &ID);
  static void AddAvailableDevices(QComboBox &combo);
 private:
  const Ui::LinuxtrackMainForm &gui;
  pref_id dev_selector;
  void Connect();
 private slots:
};


#endif