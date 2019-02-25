#ifndef LOG_VIEW__H
#define LOG_VIEW__H

#include <QWidget>
#include <QFileSystemWatcher>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QPlainTextEdit>
#include "ui_logview.h"

class LogView : public QWidget{
  Q_OBJECT
 public:
  LogView(QWidget *parent = 0);
  ~LogView();
 private slots:
  void on_CloseButton_pressed();
  void fileChanged(const QString &path);
  void readChanges();
 private:
  Ui::LogViewerForm ui;
  QFileSystemWatcher watcher;
  QFile *lf;
  QTextStream *ts;
  qint64 size;
  QTimer *timer;
  QPlainTextEdit *viewer;
  bool changed;
};

#endif

