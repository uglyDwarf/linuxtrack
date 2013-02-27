#ifndef XPLUGIN__H
#define XPLUGIN__H

#include <QDialog>
#include "ui_xplugin.h"

class XPluginInstall : public QDialog
{
  Q_OBJECT
 public:
  XPluginInstall(){ui.setupUi(this);};
 private:
  Ui::XPluginInstall ui;
 public slots:
  void on_BrowseXPlane_pressed();
};

#endif
