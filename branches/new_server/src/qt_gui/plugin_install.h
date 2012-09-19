#ifndef PLUGIN_INSTALL
#define PLUGIN_INSTALL

#include <QObject>
#include <QProcess>
#include "ui_ltr.h"

class dlfwGui;

class PluginInstall : public QObject
{
  Q_OBJECT
 public:
  PluginInstall(const Ui::LinuxtrackMainForm &ui);
  ~PluginInstall();
 private slots:
  void installWinePlugin();
  void instFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void tirFirmwareInstall();
  void tirFirmwareInstalled(bool ok);
  
 private:
  const Ui::LinuxtrackMainForm &gui;
  void Connect();
  QProcess *inst;
  dlfwGui *dlfw;
  bool isTirFirmwareInstalled();
  static const QString sigFile;
  static const QString keyFile;
  static const QString keySrc;
};


#endif

