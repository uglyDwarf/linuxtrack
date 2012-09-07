#ifndef MACWEBCAM_PREFS__H
#define MACWEBCAM_PREFS__H

#include <QObject>
#include <QComboBox>
#include "ui_m_wc_setup.h"
#include "pref.hpp"
#include "prefs_link.h"

class MacWebcamPrefs : public QWidget{
  Q_OBJECT
 public:
  MacWebcamPrefs(const QString &dev_id, QWidget *parent = 0);
  ~MacWebcamPrefs();
  static bool AddAvailableDevices(QComboBox &combo);
 private:
  const QString id;
  bool Activate(const QString &ID, bool init = false);
  Ui::MacWebcamSetupForm ui;
  bool initializing;
 private slots:
  void on_WebcamResolutionsMac_activated(int index);
  void on_WebcamThresholdMac_valueChanged(int i);
  void on_WebcamMinBlobMac_valueChanged(int i);
  void on_WebcamMaxBlobMac_valueChanged(int i);
};


#endif
