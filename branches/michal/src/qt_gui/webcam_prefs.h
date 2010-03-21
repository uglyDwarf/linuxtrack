#ifndef WEBCAM_PREFS__H
#define WEBCAM_PREFS__H

#include <QObject>
#include "ui_ltr.h"
#include "pref_int.h"

class WebcamPrefs : public QObject{
  Q_OBJECT
 public:
  WebcamPrefs(const Ui::LinuxtrackMainForm &ui);
  ~WebcamPrefs();
void Activate();
 private:
  const Ui::LinuxtrackMainForm &gui;
  pref_id dev_selector;
  void *wc_libhandle;
  void Connect();
 private slots:
  void on_WebcamIDs_currentIndexChanged(const QString &text);
  void on_WebcamFormats_currentIndexChanged(const QString &text);
  void on_WebcamResolutions_currentIndexChanged(const QString &text);
  void on_RefreshWebcams_pressed();
  void on_WebcamThreshold_valueChanged(int i);
  void on_WebcamMinBlob_valueChanged(int i);
  void on_WebcamMaxBlob_valueChanged(int i);
};


#endif