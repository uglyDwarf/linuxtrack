#ifndef LTR_DEV_HELP__H
#define LTR_DEV_HELP__H

#include "ui_dev_help.h"
#include "scp_form.h"

class LtrDevHelp : public QWidget
{
  Q_OBJECT
 public:
  LtrDevHelp(ScpForm *s, QWidget *parent = 0);
 private slots:
  void on_DumpPrefsButton_pressed();
  void on_testPitch_valueChanged(int value);
  void on_testRoll_valueChanged(int value);
  void on_testYaw_valueChanged(int value);
  void on_testX_valueChanged(int value);
  void on_testY_valueChanged(int value);
  void on_testZ_valueChanged(int value);
 private:
  Ui::DevHelp ui;
  ScpForm *scp;
};


#endif

