#ifndef TIR_PREFS__H
#define TIR_PREFS__H

#include <QObject>
#include <QComboBox>
#include "ui_tir_setup.h"
#include "prefs_link.h"
#include "extractor.h"
#include <../usb_ifc.h>

class TirPrefs : public QWidget{
  Q_OBJECT
 public:
  TirPrefs(const QString &dev_id, QWidget *parent = 0);
  ~TirPrefs();
  static bool AddAvailableDevices(QComboBox &combo);
 private:
  const QString id;
  bool Activate(const QString &ID, bool init = false);
  Ui::TirSetupForm ui;
  //void Connect();
  bool initializing;
  TirFwExtractor *dlfw;
  static bool firmwareOK;
  static bool permsOK;
 signals:
  void pressRefresh();
 private slots:
  void on_TirThreshold_valueChanged(int i);
  void on_TirMinBlob_valueChanged(int i);
  void on_TirMaxBlob_valueChanged(int i);
  void on_TirStatusBright_valueChanged(int i);
  void on_TirIrBright_valueChanged(int i);
  void on_TirSignalizeStatus_stateChanged(int state);
  void on_TirUseGrayscale_stateChanged(int state);
  void on_TirInstallFirmware_pressed();
  void TirFirmwareDLFinished(bool state);
};


#endif
