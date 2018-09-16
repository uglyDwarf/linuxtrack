#ifndef HELP_VIEWER__H
#define HELP_VIEWER__H

#include <QTextBrowser>

class QHelpEngine;
class QUrl;

class HelpViewWidget : public QTextBrowser
{
  Q_OBJECT
 public:
  HelpViewWidget(QHelpEngine *he, QWidget *parent = 0);
  QVariant loadResource(int type, const QUrl &name);
 private:
  QHelpEngine *helpEngine;
};

#endif

