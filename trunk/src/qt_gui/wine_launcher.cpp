#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include <iostream>
#include <QApplication>
#include "wine_launcher.h"

WineLauncher::WineLauncher()
{
  env = QProcessEnvironment::systemEnvironment();
  QObject::connect(&wine, SIGNAL(finished(int, QProcess::ExitStatus)), 
    this, SLOT(finished(int, QProcess::ExitStatus)));
  QObject::connect(&wine, SIGNAL(error(QProcess::ProcessError)), 
    this, SLOT(error(QProcess::ProcessError)));
}

WineLauncher::~WineLauncher()
{
  QObject::disconnect(&wine, SIGNAL(finished(int, QProcess::ExitStatus)), 
    this, SLOT(finished(int, QProcess::ExitStatus)));
  QObject::disconnect(&wine, SIGNAL(error(QProcess::ProcessError)), 
    this, SLOT(error(QProcess::ProcessError)));
  if(wine.state() != QProcess::NotRunning){
    wine.waitForFinished(10000);
  }
  if(wine.state() != QProcess::NotRunning){
    wine.kill();
  }
}

void WineLauncher::setEnv(const QString &var, const QString &val)
{
  env.insert(var, val);
}

void WineLauncher::run(const QString &tgt)
{
  wine.setProcessEnvironment(env);
  QString cmd("\"%1wine\" \"%2\"");
  QString winePath;
  #ifdef DARWIN
    winePath = QApplication::applicationDirPath()+"/../wine/bin/";
  #else
    winePath = "";
  #endif
  cmd = cmd.arg(winePath).arg(tgt);
  std::cerr<<"Launching wine command: '"<< qPrintable(cmd) <<"'"<<std::endl;
  wine.setProcessChannelMode(QProcess::MergedChannels);
  wine.start(cmd);
}

void WineLauncher::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
  (void) exitStatus;
  if(exitCode == 0 ){
    emit finished(true);
  }else{
    QString msg(wine.readAllStandardOutput());
    std::cerr<<qPrintable(msg)<<std::endl;
    emit finished(false);
  }
}

void WineLauncher::error(QProcess::ProcessError error)
{
  (void)error;
  QString msg(wine.readAllStandardOutput());
  std::cerr<<qPrintable(msg)<<std::endl;
  emit finished(false);
}


bool WineLauncher::check()
{
  run("--version");
  while(!wine.waitForFinished()){
    if(wine.error() != QProcess::Timedout){
      std::cerr<<"Process error: "<<wine.error()<<std::endl;
      return false;
    }
  }
  if(wine.exitCode() == 0){
    return true;
  }
  return false;
}


/*
void PluginInstall::installWinePlugin()
{
  gui.pushButton_2->setEnabled(false);
  QString prefix = QFileDialog::getExistingDirectory(NULL, QString("Select Wine Prefix..."), 
                     QDir::homePath()+"/.wine", QFileDialog::ShowDirsOnly);
  QString installerPath = PREF.getDataPath("linuxtrack-wine.exe");
  
  QString program = "/bin/bash";
  QString arg = QString("WINEPREFIX=") + prefix + " wine " + installerPath;
  QStringList args;
  args << "-c" << arg;
  inst->start(program, args);
}

void Extractor::extractFirmware(QString file)
{
  QString cmd("bash -c \"wine ");
  cmd += QString("%1\"").arg(file);
  progress(QString("Initializing wine and running installer %1").arg(file));
  wine->start(cmd);
}
*/
