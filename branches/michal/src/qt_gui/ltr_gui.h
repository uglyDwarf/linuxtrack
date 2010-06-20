#ifndef LTR_GUI__H
#define LTR_GUI__H

#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include <QCloseEvent>

#include "ui_ltr.h"

#ifndef DARWIN
  class WebcamPrefs;
  class WiimotePrefs;
#endif

class LtrGuiForm;
class LtrDevHelp;
class TirPrefs;
class ModelEdit;
class LtrTracking;
class LogView;
class ScpForm;

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
  void on_DeviceSelector_activated(int index);
  void on_RefreshDevices_pressed();
  void on_EditSCButton_pressed();
  void on_XplanePluginButton_pressed();
  void on_SaveButton_pressed();
  void on_ViewLogButton_pressed();
  void on_DefaultsButton_pressed();
  void on_DiscardChangesButton_pressed();
  void trackerStopped();
  void trackerRunning();
 private:
  Ui::LinuxtrackMainForm ui;
  LtrGuiForm *showWindow;
  LtrDevHelp *helper;
#ifndef DARWIN
  WebcamPrefs *wcp;
  WiimotePrefs *wiip;
#endif
  TirPrefs *tirp;
  ModelEdit *me;
  LtrTracking *track;
  ScpForm *sc;
  LogView *lv;
  bool initialized;
};


#endif