#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include "xplugin.h"
#include "ltr_gui_prefs.h"
#include "utils.h"
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QRegExp>

static QMessageBox::StandardButton warningMessage(const QString &message)
{
  ltr_int_log_message("XPlanr plugin install - %s\n", qPrintable(message));
  return QMessageBox::warning(NULL, QString::fromUtf8("Linuxtrack"),
                                message, QMessageBox::Ok);
}

static void warn(const QString baseMsg, const QString explanation)
{
  ltr_int_log_message("XPlanr plugin install - %s(%s)\n", qPrintable(baseMsg), 
                      qPrintable(explanation));
  warningMessage(QString::fromUtf8("%1\nSystem says: %2").arg(baseMsg).arg(explanation));
}

static bool removePlugin(const QString targetName)
{
  QFile target(targetName);
  if(target.exists()){
    if(!target.remove()){
      warn(QString::fromUtf8("Can't remove old plugin '%1'!").arg(targetName), target.errorString());
      return false;
    }
  }
  return true;
}


static bool installPlugin(const QString sourceFile, const QString destFile)
{
  ltr_int_log_message("Going to install '%s' to '%s'...\n", 
						qPrintable(sourceFile), qPrintable(destFile));
  //Create destination path
  QFile src(sourceFile);
  QFile dest(destFile);
  if(!src.exists()){
    warningMessage(QString::fromUtf8("Source file '%1' doesn't exist!").arg(sourceFile));
    return false;
  }
  QFileInfo destInfo(destFile);
  QDir destDir = destInfo.dir();
  //make sure the destination path exists
  if(!destDir.exists()){
    if(!destDir.mkpath(destDir.path())){
      warningMessage(QString::fromUtf8("Can't create output directory '%1'!").arg(destDir.path()));
      return false;
    }
  }
  //check if the file exists already
  if(dest.exists()){
    if(!removePlugin(destFile)){
      return false;
    }
  }
  //copy the new file
  if(!src.copy(destFile)){
    warn(QString::fromUtf8("Can't copy file '%1' to '%2'!").arg(destFile).arg(destDir.path()), src.errorString());
    return false;
  }
  return true;
}

void XPluginInstall::on_BrowseXPlane_pressed()
{
  QString fileName = QFileDialog::getOpenFileName(this,
     QString::fromUtf8("Find XPlane executable"), QDir::homePath(), QString::fromUtf8("All Files (*)"));
  QRegExp pathRexp(QString::fromUtf8("^(.*/)[^/]+$"));
  if(pathRexp.indexIn(fileName) == -1){
    reject();
    return;
  }
  QString sourceFile32 = PrefProxy::getLibPath(QString::fromUtf8("xlinuxtrack9_32"));
  QString sourceFile = PrefProxy::getLibPath(QString::fromUtf8("xlinuxtrack9"));
  QString destPath = pathRexp.cap(1) + QString::fromUtf8("/Resources/plugins");
  if(!QFile::exists(destPath)){
    warningMessage(QString(QString::fromUtf8("Can't install XPlane plugin there:'") + fileName + 
                           QString::fromUtf8("'")));
    reject();
    return;
  }
  
  //Check for the old plugin and remove it if exists
  QString oldPlugin = destPath + QString::fromUtf8("/xlinuxtrack.xpl");
  QFileInfo old(oldPlugin);
  if(old.exists()){
    if(!removePlugin(oldPlugin)){
      reject();
      return;
    }
  }
  destPath += QString::fromUtf8("/xlinuxtrack/");
#ifndef DARWIN
  if(installPlugin(sourceFile32, destPath + QString::fromUtf8("/lin.xpl")) &&
       installPlugin(sourceFile, destPath + QString::fromUtf8("/64/lin.xpl"))){
#else
  if(installPlugin(sourceFile, destPath + QString::fromUtf8("/mac.xpl")){
#endif
    QMessageBox::information(NULL, QString::fromUtf8("Linuxtrack"), 
      QString::fromUtf8("XPlane plugin installed successfuly!"));
  }else{
    warningMessage(QString::fromUtf8("XPlane plugin installation failed!"));
    reject();
  }
  accept();
}



