
#include "hotkey_setup_dlg.h"

hotKeySetupDlg::hotKeySetupDlg(QString &res, QWidget *parent) : QDialog(parent)
{
  ui.setupUi(this);
  ui.lineEdit->setTargetString(res);
}

#include "moc_hotkey_setup_dlg.cpp"

