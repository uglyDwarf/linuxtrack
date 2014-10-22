#include "log_view.h"
#include "iostream"
#include "utils.h"

LogView::LogView(QWidget *parent) : QWidget(parent), watcher(parent)
{
  ui.setupUi(this);
  setWindowTitle(QString::fromUtf8("Logfile viewer"));
  size = 0;
  changed = true;
  //To make sure the logfile exists and we can get its name
  ltr_int_log_message("Opening logfile viewer.\n");
  const char *log_name = ltr_int_get_logfile_name();
  watcher.addPath(QString::fromUtf8(log_name));
  QObject::connect(&watcher, SIGNAL(fileChanged(const QString&)), 
                   this, SLOT(fileChanged(const QString&)));
  lf = new QFile(QString::fromUtf8(log_name));
  lf->open(QIODevice::ReadOnly);
  ts = new QTextStream(lf);
  timer = new QTimer(this);
  viewer = new QPlainTextEdit(this);
  ui.verticalLayout->insertWidget(0, viewer);
  QObject::connect(timer, SIGNAL(timeout()), 
                   this, SLOT(readChanges()));
  timer->start(250);
}

void LogView::on_CloseButton_pressed()
{
  close();
}

void LogView::readChanges()
{
  if(changed)
    viewer->appendPlainText(ts->readAll());
  changed = false;
}



void LogView::fileChanged(const QString &path)
{
  (void) path;
  changed = true;
}



LogView::~LogView()
{
  timer->stop();
  delete(timer);
  delete(ts);
  delete(lf);
  ui.verticalLayout->removeWidget(viewer);
  delete(viewer);
}
