#ifndef HOTKEY_DIALOG__H
#define HOTKEY_DIALOG__H

#include "ui_hotkey_setup.h"

class hotKeySetupDlg : public QDialog
{
  Q_OBJECT
 public:
  hotKeySetupDlg(QString &res, QWidget *parent = 0);
 private:
  Ui::HotKeySetupDlg ui;
};


#endif