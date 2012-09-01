#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include "pref.hpp"
#include "pref_global.h"
#include "ltr_gui_prefs.h"
#include "map"
#include "utils.h"

#include <QStringList>
#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <iostream>

PrefProxy *PrefProxy::prf = NULL;
prefs &PrefProxy::ltrPrefs = prefs::getPrefs();

static int warnMessage(const QString &message){
 return QMessageBox::warning(NULL, "Linuxtrack",
                                message, QMessageBox::Ok, QMessageBox::Ok);
}

PrefProxy::PrefProxy()
{
  if(ltr_int_read_prefs(NULL, false)){
    checkPrefix(true);
    return;
  }
  ltr_int_log_message("Couldn't load preferences (GUI)!\n");
  if(!makeRsrcDir()){
    throw;
  }
  if(!copyDefaultPrefs()){
    throw;
  }
  ltr_int_new_prefs();
  ltr_int_read_prefs(NULL, true);
  checkPrefix(true);
}

bool PrefProxy::checkPrefix(bool save)
{
  QString appPath = QApplication::applicationDirPath();
  appPath.prepend("\"");
  appPath += "\"";
  std::string tmp_prefix("");
  bool res = ltrPrefs.getValue("Global", "Prefix", tmp_prefix);
  prefix = QString::fromStdString(tmp_prefix);
  if(res && (prefix == appPath)){
    //Intentionaly left empty
  }else{
    prefix = appPath;
    bool res = true;
    ltrPrefs.setValue("Global", "Prefix", appPath.toStdString());
    if(save){
      res &= savePrefs();
    }
    return res;
  }
  return true;
}

bool PrefProxy::makeRsrcDir()
{
  QString msg;
  QString targetDir = PrefProxy::getRsrcDirPath();
  if(targetDir.endsWith("/")){
    targetDir.chop(1);
  }
  QFileInfo rsrcDir(targetDir);
  if(rsrcDir.isDir()){
    return true;
  }
  if(rsrcDir.exists() || rsrcDir.isSymLink()){
    QString bck = targetDir + ".pre";
    QFileInfo bckInfo(bck);
    if(bckInfo.exists() || bckInfo.isSymLink()){
      if(!QFile::remove(bck)){
        msg = QString("Can't remove '" + bck + "'!");
        goto problem;
      }
    }
    if(!QFile::rename(targetDir, bck)){
      msg = QString("Can't rename '" + targetDir + "' to '" + bck + "'!");
      goto problem;
    }
  }
  if(!QDir::home().mkpath(targetDir)){
    msg = QString("Can't create '" + targetDir + "'!");
    goto problem;
  }
  return true;
 problem:
  ltr_int_log_message(QString(msg+"\n").toAscii().data());
  warnMessage(msg);
  return false;  
}


bool PrefProxy::copyDefaultPrefs()
{
  QString msg;
  //we can assume the rsrc dir exists now...
  QString targetDir = PrefProxy::getRsrcDirPath();
  if(targetDir.endsWith("/")){
    targetDir.chop(1);
  }
  QString target = targetDir + "/linuxtrack.conf";
  QString source = PrefProxy::getDataPath("linuxtrack.conf");
  QFileInfo target_info(target);
  if(target_info.exists() || target_info.isSymLink()){
    QString bck = target + ".backup";
    QFileInfo bckInfo(bck);
    if(bckInfo.exists() || bckInfo.isSymLink()){
      if(!QFile::remove(bck)){
        msg = QString("Can't remove '" + bck + "'!");
        goto problem;
      }
    }
    if(!QFile::rename(target, bck)){
      msg = QString("Can't rename '" + target + "' to '" + bck + "'!");
      goto problem;
    }
  }
  if(!QFile::copy(source, target)){
    msg = QString("Can't copy '" + source + "' to '" + target + "'!");
    goto problem;
  }
  
  return true;
 problem:
  ltr_int_log_message(QString(msg+"\n").toAscii().data());
  warnMessage(msg);
  return false;  
}


PrefProxy::~PrefProxy()
{
}

PrefProxy& PrefProxy::Pref()
{
  if(prf == NULL){
    prf = new PrefProxy();
  }
  return *prf;
}

void PrefProxy::ClosePrefs()
{
  if(prf != NULL){
    delete prf;
    prf = NULL;
    ltr_int_free_prefs();
  }
}

void PrefProxy::SavePrefsOnExit()
{
  if(ltrPrefs.changed()){
    QMessageBox::StandardButton res;
    res = QMessageBox::warning(NULL, "Linuxtrack",
       QString("Preferences were modified,") +
       QString("Do you want to save them?"), 
       QMessageBox::Save | QMessageBox::Close, QMessageBox::Save);
    if(res == QMessageBox::Save){
      savePrefs();
    }
  }
}

bool PrefProxy::activateDevice(const QString &sectionName)
{
  ltrPrefs.setValue("Global", "Input", sectionName.toStdString());
  return true;
}

bool PrefProxy::activateModel(const QString &sectionName)
{
  ltrPrefs.setValue("Global", "Model", sectionName.toStdString());
  return true;
}

bool PrefProxy::createSection(QString 
&sectionName)
{
  int i = 0;
  QString newSecName = sectionName;
  while(1){
    if(!ltrPrefs.sectionExists(newSecName.toStdString())){
      break;
    }
    newSecName = QString("%1_%2").arg(sectionName).
                                           arg(QString::number(++i));
  }
  ltrPrefs.addSection(newSecName.toStdString());
  sectionName = newSecName;
  return true;
}

bool PrefProxy::getKeyVal(const QString &sectionName, const QString &keyName, 
			  QString &result)
{
  std::string val;
  if(ltrPrefs.getValue(sectionName.toStdString(), keyName.toStdString(), val)){
    result = QString::fromStdString(val);
    return true;
  }else{
    return false;
  }
}

/*
bool PrefProxy::getKeyVal(const QString &keyName, QString &result)
{
  const char *val = ltr_int_get_key(NULL, keyName.toAscii().data());
  if(val != NULL){
    result = val;
    return true;
  }else{
    return false;
  }
}
*/

bool PrefProxy::addKeyVal(const QString &sectionName, const QString &keyName, 
			  const QString &value)
{
  ltrPrefs.addKey(sectionName.toStdString(), keyName.toStdString(), 
		 value.toStdString());
  return true;
}

bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
			  const QString &value)
{
  ltrPrefs.setValue(sectionName.toStdString(), keyName.toStdString(),
			    value.toStdString());
  return true;
}

bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
                          const int &value)
{
  ltrPrefs.setValue(sectionName.toStdString(), keyName.toStdString(), value);
  return true;
}

bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
                          const float &value)
{
  ltrPrefs.setValue(sectionName.toStdString(), keyName.toStdString(), value);
  return true;
}

bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
                          const double &value)
{
  ltrPrefs.setValue(sectionName.toStdString(), keyName.toStdString(), (float)value);
  return true;
}

bool PrefProxy::getFirstDeviceSection(const QString &devType, QString &result)
{
  std::vector<std::string> sections;
  ltrPrefs.getSectionList(sections);
  std::string devName;
  for(size_t i = 0; i < sections.size(); ++i){
    if(ltrPrefs.getValue(sections[i], "Capture-device", devName)){
      if(QString::fromStdString(devName) == devType){
	result = QString::fromStdString(sections[i]);
	return true;
      }
    }
  }
  return false;
}

bool PrefProxy::getFirstDeviceSection(const QString &devType, 
				      const QString &devId, QString &result)
{
  std::vector<std::string> sections;
  ltrPrefs.getSectionList(sections);
  std::string devName, devIdStr;
  for(size_t i = 0; i < sections.size(); ++i){
    if(ltrPrefs.getValue(sections[i], "Capture-device", devName) &&
       ltrPrefs.getValue(sections[i], "Capture-device-id", devIdStr)){
      if((QString::fromStdString(devName) == devType) && 
         (QString::fromStdString(devIdStr) == devId)){
	result = QString::fromStdString(sections[i]);
	return true;
      }
    }
  }
  return false;
}

bool PrefProxy::getActiveDevice(deviceType_t &devType, QString &id, QString &secName)
{
  std::string devSection, devName, devId;
  if(!ltrPrefs.getValue("Global", "Input", devSection)){
    return false;
  }
  if(!ltrPrefs.getValue(devSection, "Capture-device", devName) ||
     !ltrPrefs.getValue(devSection, "Capture-device-id", devId)){
    return false;
  }
  QString dn = QString::fromStdString(devName);
  if(dn.compare((char *)"Webcam", Qt::CaseInsensitive) == 0){
    devType = WEBCAM;
  }else if(dn.compare((char *)"Webcam-face", Qt::CaseInsensitive) == 0){
    devType = WEBCAM_FT;
  }else if(dn.compare((char *)"Wiimote", Qt::CaseInsensitive) == 0){
    devType = WIIMOTE;
  }else if(dn.compare((char *)"Tir", Qt::CaseInsensitive) == 0){
    devType = TIR;
  }else if(dn.compare((char *)"Tir_openusb", Qt::CaseInsensitive) == 0){
    devType = TIR;
  }else{
    devType = NONE;
  }
  id = QString::fromStdString(devId);
  secName = QString::fromStdString(devSection);
  return true;
}

bool PrefProxy::getActiveDevice(deviceType_t &devType, QString &id)
{
  QString tmp;
  return getActiveDevice(devType, id, tmp);
}

bool PrefProxy::getActiveModel(QString &model)
{
  std::string modelSection;
  if(!ltrPrefs.getValue("Global", "Model", modelSection)){
    return false;
  }
  model = QString::fromStdString(modelSection);
  return true;
}

bool PrefProxy::getModelList(QStringList &list)
{
  std::vector<std::string> sections;
  std::string modelType;
  ltrPrefs.getSectionList(sections);
  list.clear();
  for(size_t i = 0; i < sections.size(); ++i){
    if(ltrPrefs.getValue(sections[i], "Model-type", modelType)){
      list.append(QString::fromStdString(sections[i]));
    }
  }
  return (list.size() != 0);
}

bool PrefProxy::getProfiles(QStringList &list)
{
  std::vector<std::string> sections;
  std::string profileName;
  ltrPrefs.getSectionList(sections);
  list.clear();
  for(size_t i = 0; i < sections.size(); ++i){
    if(ltrPrefs.getValue(sections[i], "Title", profileName)){
      list.append(QString::fromStdString(profileName));
    }
  }
  return (list.size() != 0);
}

bool PrefProxy::getProfileSection(const QString &name, QString &section)
{
  std::string secName;
  if(ltrPrefs.findSection("Title", name.toStdString(), secName)){
    section = QString::fromStdString(secName);
    return true;
  }
  return false;
}

bool PrefProxy::savePrefs()
{
  bool res = ltr_int_save_prefs(NULL);
  return res;
}

QString PrefProxy::getDataPath(QString file)
{
  char *path = ltr_int_get_data_path_prefix(file.toStdString().c_str(), 
                                            QApplication::applicationDirPath().toStdString().c_str());
  QString res = path;
  free(path);
  return res; 
/*  
  QString appPath = QApplication::applicationDirPath();
#ifndef DARWIN
  return appPath + "/../share/linuxtrack/" + file;
#else
  return appPath + "/../Resources/linuxtrack/" + file;
#endif
*/
}

QString PrefProxy::getLibPath(QString file)
{
  char *path = ltr_int_get_lib_path(file.toStdString().c_str());
  QString res = path;
  free(path);
  return res;   
/*
  QString appPath = QApplication::applicationDirPath();
#ifndef DARWIN
  return appPath + "/../lib/" + file + ".so.0";
#else
  return appPath + "/../Frameworks/" + file + ".0.dylib";
#endif
*/
}

QString PrefProxy::getRsrcDirPath()
{
  return QDir::homePath() + "/.linuxtrack/";
}

bool PrefProxy::rereadPrefs()
{
  ltr_int_free_prefs();
  ltr_int_read_prefs(NULL, true);
  checkPrefix(true);
  return true;
}

void PrefProxy::announceModelChange()
{
  ltr_int_announce_model_change();
}

