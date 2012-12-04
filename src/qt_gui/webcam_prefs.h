#ifndef WEBCAM_PREFS__H
#define WEBCAM_PREFS__H

#include <QObject>
#include <QComboBox>
#include "ui_l_wc_setup.h"
#include "pref.hpp"
#include "prefs_link.h"

class WebcamPrefs : public QWidget{
  Q_OBJECT
 public:
  WebcamPrefs(const QString &dev_id, QWidget *parent = 0);
  ~WebcamPrefs();
  static bool AddAvailableDevices(QComboBox &combo);
 private:
  const QString id;
  bool Activate(const QString &ID, bool init = false);
  Ui::WebcamSetupForm ui;
  bool initializing;
 private slots:
  void on_WebcamFormats_activated(int index);
  void on_WebcamResolutions_activated(int index);
  void on_WebcamThreshold_valueChanged(int i);
  void on_WebcamMinBlob_valueChanged(int i);
  void on_WebcamMaxBlob_valueChanged(int i);
};


#endif
