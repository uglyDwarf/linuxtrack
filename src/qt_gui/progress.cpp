
#include "progress.h"

void Progress::message(qint64 read, qint64 all)
{
  ui.ProgressBar->setValue((float)read / all * 100.0);
  ui.InfoLabel->setText(QString::fromUtf8("Downloaded %1 of %2.").arg(read).arg(all));
}

#include "moc_progress.cpp"

