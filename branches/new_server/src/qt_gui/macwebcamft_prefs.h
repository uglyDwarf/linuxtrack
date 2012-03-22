#ifndef MACWEBCAMFT_PREFS__H
#define MACWEBCAMFT_PREFS__H

#include <QObject>
#include <QComboBox>
#include <QFileDialog>
#include "ui_ltr.h"
#include "pref_int.h"
#include "prefs_link.h"

class WebcamFtPrefs : public QObject{
  Q_OBJECT
 public:
  WebcamFtPrefs(const Ui::LinuxtrackMainForm &ui);
  ~WebcamFtPrefs();
  bool Activate(const QString &ID, bool init = false);
  static bool AddAvailableDevices(QComboBox &combo);
 private:
  const Ui::LinuxtrackMainForm &gui;
  void Connect();
  bool initializing;
  bool prefInit;
 private slots:
  void on_WebcamResolutions_activated(int index);
  void on_FindCascade_pressed();
  void on_CascadePath_editingFinished();
  void on_ExpFilterFactor_valueChanged(int value);
  void on_OptimLevel_valueChanged(int value);
};


#endif