#ifndef LTR_GUI__H
#define LTR_GUI__H

#include <QCloseEvent>

#include "ui_ltr.h"
#include "webcam_prefs.h"
#include "wiimote_prefs.h"
#include "tir_prefs.h"
#include "ltr_show.h"
#include "ltr_dev_help.h"

class LinuxtrackGui : public QWidget
{
  Q_OBJECT
 public:
  LinuxtrackGui(QWidget *parent = 0);
  ~LinuxtrackGui();
protected:
     void closeEvent(QCloseEvent *event);
 private slots:
  void on_QuitButton_pressed();
  void on_CreateModelButton_pressed();
  void on_DeviceSelector_activated(int index);
  void on_RefreshDevices_pressed();
 private:
  Ui::LinuxtrackMainForm ui;
  LtrGuiForm showWindow;
  LtrDevHelp helper;
  WebcamPrefs *wcp;
  WiimotePrefs *wiip;
  TirPrefs *tirp;
};


#endif