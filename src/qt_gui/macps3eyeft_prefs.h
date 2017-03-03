#ifndef MACPS3EYEFT_PREFS__H
#define MACPS3EYEFT_PREFS__H

#include <QObject>
#include <QComboBox>
#include "ui_m_ps3eye_ft_setup.h"
#include "pref.hpp"
#include "prefs_link.h"

class MacP3eFtPrefs : public QWidget{
  Q_OBJECT
 public:
  MacP3eFtPrefs(const QString &dev_id, QWidget *parent = 0);
  ~MacP3eFtPrefs();
  static bool AddAvailableDevices(QComboBox &combo);
 private:
  const QString id;
  bool Activate(const QString &ID, bool init = false);
  Ui::MacPS3FtSetupForm ui;
  bool initializing;
 private slots:
  void on_WebcamResolutionsMac_activated(int index);
  void on_EXPOSURE_valueChanged(int i);
  void on_GAIN_valueChanged(int i);
  void on_BRIGHTNESS_valueChanged(int i);
  void on_CONTRAST_valueChanged(int i);
  void on_SHARPNESS_valueChanged(int i);
  void on_AGC_stateChanged(int state);
  void on_AWB_stateChanged(int state);
  void on_AEX_stateChanged(int state);
  void on_PLF50_stateChanged(int state);
  void on_FindCascadeMac_pressed();
  void on_CascadePathMac_editingFinished();
  void on_ExpFilterFactorMac_valueChanged(int value);
  void on_OptimLevelMac_valueChanged(int value);
};


#endif
