#ifndef PLUGIN_INSTALL
#define PLUGIN_INSTALL

#include <QObject>
#include <QProcess>
#include "ui_ltr.h"

class PluginInstall : public QObject
{
  Q_OBJECT
 public:
  PluginInstall(const Ui::LinuxtrackMainForm &ui);
  ~PluginInstall();
 private slots:
  void installWinePlugin();
  void instFinished(int exitCode, QProcess::ExitStatus exitStatus);
 private:
  const Ui::LinuxtrackMainForm &gui;
  void Connect();
  QProcess *inst;
};


#endif

