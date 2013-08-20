#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include <iostream>
#include <sstream>
#include <QApplication>
#include "wine_launcher.h"
#include "utils.h"

void WineLauncher::envSet(const QString var, const QString val)
{
    env.insert(var, val);
    std::ostringstream s;
    s<<"    "<<qPrintable(var)<<"='"<<qPrintable(val)<<"'"<<std::endl;
    ltr_int_log_message(s.str().c_str());
}

WineLauncher::WineLauncher():winePath(""), available(false)
{
  std::ostringstream s;
  env = QProcessEnvironment::systemEnvironment();
  if(!check()){
#ifdef DARWIN
    winePath = QApplication::applicationDirPath()+"/../wine/bin/";
    QString libPath = QApplication::applicationDirPath()+"/../wine/lib/";
    available = true;
    QString path = winePath + ":" + env.value("PATH");
    s.str(std::string(""));
    s<<"Using internal wine; adjusting env variables:"<<std::endl;
    ltr_int_log_message(s.str().c_str());
    envSet("PATH", path);
    envSet("WINESERVER", winePath+"wineserver");
    envSet("WINELOADER", winePath+"wine");
    envSet("WINEDLLPATH", libPath+"wine/fakedlls");
    envSet("DYLD_LIBRARY_PATH", libPath);
    envSet("DYLD_PRINT_ENV", "1");
    envSet("DYLD_PRINT_LIBRARIES", "1");
    envSet("WINEDEBUG", "+file,+seh,+tid,+process,+rundll,+module");
#endif
  }else{
    available = true;
  }
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
  cmd = cmd.arg(winePath).arg(tgt);
  std::ostringstream s;
  s<<"Launching wine command: '"<< qPrintable(cmd) <<"'"<<std::endl;
  ltr_int_log_message(s.str().c_str());
  wine.setProcessChannelMode(QProcess::MergedChannels);
  wine.start(cmd);
}

void WineLauncher::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
  QString status;
  switch(exitStatus){
    case QProcess::NormalExit:
      status = "Normal exit";
      break;
    case QProcess::CrashExit:
      status = "Crashed";
      break;
    default:
      status = "Unknown exit status";
      break;
  }
  ltr_int_log_message("Wine finished with exitcode %d (%s).", exitCode, qPrintable(status));
  QString msg(wine.readAllStandardOutput());
  std::ostringstream s;
  s<<qPrintable(msg)<<std::endl;
  ltr_int_log_message(s.str().c_str());
  if(exitCode == 0 ){
    emit finished(true);
  }else{
    emit finished(false);
  }
}

const QString errorStr(QProcess::ProcessError error)
{  
  QString reason;
  switch(error){
    case QProcess::FailedToStart:
      reason ="Failed To Start";
      break;
    case QProcess::Crashed:
      reason = "Crashed";
      break;
    case QProcess::Timedout:
      reason = "Timedout";
      break;
    case QProcess::WriteError:
      reason = "Write Error";
      break;
    case QProcess::ReadError:
      reason = "Read Error";
      break;
    default:
      reason = "Unknown Error";
      break;
  }
  return reason;
}

void WineLauncher::error(QProcess::ProcessError error)
{
  QString msg(wine.readAllStandardOutput());
  QString reason = errorStr(error);
  ltr_int_log_message("Error launching wine(%s)!", qPrintable(reason));
  std::ostringstream s;
  s<<qPrintable(msg)<<std::endl;
  ltr_int_log_message(s.str().c_str());
  emit finished(false);
}

bool WineLauncher::wineAvailable()
{
  return available;
}


bool WineLauncher::check()
{
  run("--version");
  while(!wine.waitForFinished()){
    if(wine.error() != QProcess::Timedout){
      std::ostringstream s;
      s<<"Process error: "<<qPrintable(errorStr(wine.error()))<<std::endl;
      ltr_int_log_message(s.str().c_str());
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
