#ifndef PROGRESS__H
#define PROGRESS__H

#include "ui_progress.h"
#include <QDialog>

class Progress: public QDialog
{
 Q_OBJECT
 public:
  Progress(){ui.setupUi(this); ui.InfoLabel->setText(QString::fromUtf8(""));};
  void show(){ui.ProgressBar->setValue(0);QWidget::show();};
 private:
  Ui::DLProgress ui;
 public slots:
  void message(qint64 read, qint64 all);
};

#endif
