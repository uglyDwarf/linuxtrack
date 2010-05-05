#include "log_view.h"


LogView::LogView(QWidget *parent) : QWidget(parent), watcher(parent)
{
  ui.setupUi(this);
  watcher.addPath("/tmp/linuxtrack.log");
  QObject::connect(&watcher, SIGNAL(fileChanged(const QString&)), 
                   this, SLOT(fileChanged(const QString&)));
  lf = new QFile("/tmp/linuxtrack.log");
  lf->open(QIODevice::ReadOnly);
  ts = new QTextStream(lf);
}

void LogView::on_CloseButton_pressed()
{
  close();
}

void LogView::fileChanged(const QString &path)
{
  (void) path;
  ts->seek(0);
  
  ui.LogViewer->setPlainText("");
  ui.LogViewer->appendPlainText(ts->readAll());
}

LogView::~LogView()
{
  delete(ts);
  delete(lf);
}