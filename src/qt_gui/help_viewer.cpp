#include <help_viewer.h>
#include <QHelpEngine>
HelpViewWidget::HelpViewWidget(QHelpEngine *he, QWidget *parent):QTextBrowser(parent), helpEngine(he)
{
}

QVariant HelpViewWidget::loadResource(int type, const QUrl &name)
{
  if(name.scheme() == QString::fromUtf8("qthelp")){
    return QVariant(helpEngine->fileData(name));
  }else{
    return QTextBrowser::loadResource(type, name);
  }
}


