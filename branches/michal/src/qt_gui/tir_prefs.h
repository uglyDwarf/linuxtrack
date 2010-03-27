#ifndef TIR_PREFS__H
#define TIR_PREFS__H

#include <QObject>
#include <QComboBox>
#include "ui_ltr.h"
#include "pref_int.h"
#include "prefs_link.h"

class TirPrefs : public QObject{
  Q_OBJECT
 public:
  TirPrefs(const Ui::LinuxtrackMainForm &ui);
  ~TirPrefs();
  void Activate(const QString &ID);
  static void AddAvailableDevices(QComboBox &combo);
 private:
  const Ui::LinuxtrackMainForm &gui;
  void Connect();
 private slots:
  void on_TirThreshold_valueChanged(int i);
  void on_TirMinBlob_valueChanged(int i);
  void on_TirMaxBlob_valueChanged(int i);
  void on_TirStatusBright_valueChanged(int i);
  void on_TirIrBright_valueChanged(int i);
  void on_TirSignalizeStatus_stateChanged(int state);
};


#endif