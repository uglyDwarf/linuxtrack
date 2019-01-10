#ifndef WINE_WARN__H
#define WINE_WARN__H

#include <QWidget>
#include "ui_wine_warn.h"

class WineWarn : public QDialog
{
 Q_OBJECT
 public:
  WineWarn(QWidget *parent = 0);
  ~WineWarn();
 private:
  Ui::Dialog ui;
 public slots:
  void show();
  void on_OKButton_pressed();
};

#endif
