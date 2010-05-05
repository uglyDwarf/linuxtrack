#include <QWidget>
#include <QFileSystemWatcher>
#include <QFile>
#include <QTextStream>
#include "ui_logview.h"

class LogView : public QWidget{
  Q_OBJECT
 public:
  LogView(QWidget *parent = 0);
  ~LogView();
 private slots:
  void on_CloseButton_pressed();
  void fileChanged(const QString &path);
 private:
  Ui::LogViewerForm ui;
  QFileSystemWatcher watcher;
  QFile *lf;
  QTextStream *ts;
};