#include "log_view.h"
#include "iostream"

LogView::LogView(QWidget *parent) : QWidget(parent), watcher(parent)
{
  ui.setupUi(this);
  size = 0;
  changed = true;
  watcher.addPath("/tmp/ltr_gui.log");
  QObject::connect(&watcher, SIGNAL(fileChanged(const QString&)), 
                   this, SLOT(fileChanged(const QString&)));
  lf = new QFile("/tmp/ltr_gui.log");
  lf->open(QIODevice::ReadOnly);
  ts = new QTextStream(lf);
  timer = new QTimer(this);
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
    ui.LogViewer->appendPlainText(ts->readAll());
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
}