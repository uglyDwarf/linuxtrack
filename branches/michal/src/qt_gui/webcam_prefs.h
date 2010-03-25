#ifndef WEBCAM_PREFS__H
#define WEBCAM_PREFS__H

#include <QObject>
#include <QComboBox>
#include "ui_ltr.h"
#include "pref_int.h"
#include "prefs_link.h"

class WebcamPrefs : public QObject{
  Q_OBJECT
 public:
  WebcamPrefs(const Ui::LinuxtrackMainForm &ui);
  ~WebcamPrefs();
  void Activate(const QString &ID);
  static void AddAvailableDevices(QComboBox &combo);
 private:
  const Ui::LinuxtrackMainForm &gui;
  void Connect();
 private slots:
  void on_WebcamFormats_activated(int index);
  void on_WebcamResolutions_activated(int index);
  void on_WebcamThreshold_valueChanged(int i);
  void on_WebcamMinBlob_valueChanged(int i);
  void on_WebcamMaxBlob_valueChanged(int i);
};


#endif