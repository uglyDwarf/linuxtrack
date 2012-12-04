#ifndef MACWEBCAMFT_PREFS__H
#define MACWEBCAMFT_PREFS__H

#include <QObject>
#include <QComboBox>
#include <QFileDialog>
#include "ui_m_wcft_setup.h"
#include "pref.hpp"
#include "prefs_link.h"

class MacWebcamFtPrefs : public QWidget{
  Q_OBJECT
 public:
  MacWebcamFtPrefs(const QString &dev_id, QWidget *parent = 0);
  ~MacWebcamFtPrefs();
  static bool AddAvailableDevices(QComboBox &combo);
 private:
  bool Activate(const QString &ID, bool init = false);
  const QString id;
  Ui::MacWebcamFtSetupForm ui;
  bool initializing;
  bool prefInit;
 private slots:
  void on_WebcamFtResolutionsMac_activated(int index);
  void on_FindCascadeMac_pressed();
  void on_CascadePathMac_editingFinished();
  void on_ExpFilterFactorMac_valueChanged(int value);
  void on_OptimLevelMac_valueChanged(int value);
};


#endif
