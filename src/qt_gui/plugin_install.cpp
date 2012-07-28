#include <plugin_install.h>
#include <ltr_gui_prefs.h>
#include <iostream>
#include <QProcess>
#include <QFileDialog>

PluginInstall::PluginInstall(const Ui::LinuxtrackMainForm &ui, QWidget *parent):
  QWidget(parent), gui(ui), inst(NULL)
{
  if(QFile::exists(PREF.getDataPath("linuxtrack-wine.exe"))){
    inst = new QProcess(this);
    gui.pushButton_2->setEnabled(true);
    Connect();
  }
}

PluginInstall::~PluginInstall()
{
  delete inst;
}

void PluginInstall::Connect()
{
  QObject::connect(gui.pushButton_2, SIGNAL(pressed()),
    this, SLOT(installWinePlugin()));
  QObject::connect(inst, SIGNAL(finished(int, QProcess::ExitStatus)),
    this, SLOT(instFinished(int, QProcess::ExitStatus)));
}

void PluginInstall::instFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  (void) exitCode;
  (void) exitStatus;
  gui.pushButton_2->setEnabled(true);
}

void PluginInstall::installWinePlugin()
{
  gui.pushButton_2->setEnabled(false);
  QString prefix = QFileDialog::getExistingDirectory(this, QString("Select Wine Prefix..."), 
                     QDir::homePath()+"/.wine", QFileDialog::ShowDirsOnly);
  QString installerPath = PREF.getDataPath("linuxtrack-wine.exe");
  
  QString program = "/bin/bash";
  QString arg = QString("WINEPREFIX=") + prefix + " wine " + installerPath;
  QStringList args;
  args << "-c" << arg;
  inst->start(program, args);
}


