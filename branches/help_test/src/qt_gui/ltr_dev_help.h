#ifndef LTR_DEV_HELP__H
#define LTR_DEV_HELP__H

#include "ui_dev_help.h"

class LtrDevHelp : public QWidget
{
  Q_OBJECT
 public:
  LtrDevHelp(QWidget *parent = 0);
 private slots:
  void on_DumpPrefsButton_pressed();
 private:
  Ui::DevHelp ui;
};


#endif

