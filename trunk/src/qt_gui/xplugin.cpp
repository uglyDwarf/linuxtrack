#include "xplugin.h"
#include "ltr_gui_prefs.h"

#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QRegExp>

static QMessageBox::StandardButton warningMessage(const QString &message)
{
 return QMessageBox::warning(NULL, "Linuxtrack",
                                message, QMessageBox::Ok);
}

static void warn(const QString baseMsg, const QString explanation)
{
  warningMessage(QString("%1\nSystem says: %2").arg(baseMsg).arg(explanation));
}

static bool removePlugin(const QString targetName)
{
  QFile target(targetName);
  if(target.exists()){
    if(!target.remove()){
      warn(QString("Can't remove old plugin '%1'!").arg(targetName), target.errorString());
      return false;
    }
  }
  return true;
}


static bool installPlugin(const QString sourceFile, const QString destFile)
{
  //Create destination path
  QFile src(sourceFile);
  QFile dest(destFile);
  if(!src.exists()){
    warningMessage(QString("Source file '%s' doesn't exist!"));
    return false;
  }
  QFileInfo destInfo(destFile);
  QDir destDir = destInfo.dir();
  //make sure the destination path exists
  if(!destDir.exists()){
    if(!destDir.mkpath(destDir.path())){
      warningMessage(QString("Can't create output directory '%1'!").arg(destDir.path()));
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
    warn(QString("Can't copy file '%1' to '%2'!").arg(destFile).arg(destDir.path()), src.errorString());
    return false;
  }
  return true;
}

void XPluginInstall::on_BrowseXPlane_pressed()
{
  QString fileName = QFileDialog::getOpenFileName(this,
     "Find XPlane executable", QDir::homePath(), "All Files (*)");
  QRegExp pathRexp("^(.*/)[^/]+$");
  if(pathRexp.indexIn(fileName) == -1){
    hide();
    return;
  }
  QString sourceFile32 = PrefProxy::getLibPath("xlinuxtrack9_32");
  QString sourceFile = PrefProxy::getLibPath("xlinuxtrack9");
  QString destPath = pathRexp.cap(1) + "/Resources/plugins";
  if(!QFile::exists(destPath)){
    warningMessage(QString("This doesn't seem to be the right path... '" + fileName + "'"));
    return;
  }
  
  //Check for the old plugin and remove it if exists
  QString oldPlugin = destPath + "/xlinuxtrack.xpl";
  QFileInfo old(oldPlugin);
  if(old.exists()){
    if(!removePlugin(oldPlugin)){
      return;
    }
  }
  destPath += "/xlinuxtrack/";
#ifndef DARWIN
  if(installPlugin(sourceFile32, destPath + "/lin.xpl") &&
       installPlugin(sourceFile, destPath + "/64/lin.xpl")){
#else
  if(installPlugin(sourceFile, destPath + "/mac.xpl")){
#endif
    QMessageBox::information(NULL, "Linuxtrack", "XPlane plugin installed successfuly!");
  }else{
    warningMessage(QString("XPlane plugin installation failed!"));
  }
  hide();
}



