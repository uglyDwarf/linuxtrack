#ifndef LTR_GUI__H
#define LTR_GUI__H

#include "pref_int.h"
#include "ui_ltr.h"
#include "webcam_prefs.h"

class LinuxtrackGui : public QWidget
{
  Q_OBJECT
 public:
  LinuxtrackGui(QWidget *parent = 0);
 private slots:
  void on_QuitButton_pressed();
//  void on_WebcamIDs_currentIndexChanged(const QString &text);
//  void on_WebcamFormats_currentIndexChanged(const QString &text);
//  void on_WebcamResolutions_currentIndexChanged(const QString &text);
  void on_DeviceSelector_currentIndexChanged(const QString &text);
 private:
  Ui::LinuxtrackMainForm ui;
  WebcamPrefs *wcp;
  pref_id dev_selector;
};


#endif