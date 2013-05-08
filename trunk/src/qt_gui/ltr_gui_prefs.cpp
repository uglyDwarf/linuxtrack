#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include "pref.h"
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
  ltr_int_log_message("Pref file not found, trying linuxtrack.conf\n");
  if(ltr_int_read_prefs("linuxtrack.conf", false)){
    ltr_int_prefs_changed();
    checkPrefix(true);
    return;
  }
  
  ltr_int_log_message("Couldn't load preferences (GUI), copying default!\n");
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
  bool res;
  char *tmp_prefix = ltr_int_get_key("Global", "Prefix");
  if(tmp_prefix != NULL){
    res = true;
    prefix = QString::fromStdString(tmp_prefix);
  }else{
    res = false;
    prefix = QString("");
  }
  if(res && (prefix == appPath)){
    //Intentionaly left empty
  }else{
    prefix = appPath;
    bool res = ltr_int_change_key("Global", "Prefix",qPrintable(appPath));
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
  QString target = targetDir + "/linuxtrack1.conf";
  QString source = PrefProxy::getDataPath("linuxtrack1.conf");
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
  if(ltr_int_need_saving()){
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
  ltr_int_change_key("Global", "Input", qPrintable(sectionName));
  return true;
}

bool PrefProxy::activateModel(const QString &sectionName)
{
  ltr_int_change_key("Global", "Model", qPrintable(sectionName));
  return true;
}

bool PrefProxy::createSection(QString 
&sectionName)
{
  char *tmp = ltr_int_add_unique_section(qPrintable(sectionName));
  if(tmp != NULL){
    sectionName = tmp;
    return true;
  }else{
    sectionName = "";
    return false;
  }
}

bool PrefProxy::getKeyVal(const QString &sectionName, const QString &keyName, 
			  QString &result)
{
  char *val = ltr_int_get_key(qPrintable(sectionName), qPrintable(keyName));
  if(val != NULL){
    result = val;
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
  return ltr_int_change_key(qPrintable(sectionName), qPrintable(keyName), 
		 qPrintable(value));
}

bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
			  const QString &value)
{
  return ltr_int_change_key(qPrintable(sectionName), qPrintable(keyName), 
		 qPrintable(value));
}

bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
                          const int &value)
{
  return ltr_int_change_key_int(qPrintable(sectionName), qPrintable(keyName), value);
}

bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
                          const float &value)
{
  return ltr_int_change_key_flt(qPrintable(sectionName), qPrintable(keyName), value);
}

bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
                          const double &value)
{
  return ltr_int_change_key_flt(qPrintable(sectionName), qPrintable(keyName), value);
}

bool PrefProxy::getFirstDeviceSection(const QString &devType, QString &result)
{
  char *devName = ltr_int_find_section("Capture-device", qPrintable(devType));
  if(devName != NULL){
    result = QString(devName);
    return true;
  }else{
    return false;
  }
}

bool PrefProxy::getFirstDeviceSection(const QString &devType, 
				      const QString &devId, QString &result)
{
  QStringList sections;
  getSectionList(sections);
  char *devName, *devIdStr;
  for(ssize_t i = 0; i < sections.size(); ++i){
    devName = ltr_int_get_key(qPrintable(sections[i]), "Capture-device");
    devIdStr = ltr_int_get_key(qPrintable(sections[i]), "Capture-device-id");
    if((devName != NULL) && (devIdStr != NULL)){
      if((devType.compare(devName, Qt::CaseInsensitive) == 0) 
         && (devId.compare(devIdStr, Qt::CaseInsensitive) == 0)){
	result = QString(sections[i]);
	return true;
      }
    }
  }
  return false;
}

bool PrefProxy::getActiveDevice(deviceType_t &devType, QString &id, QString &secName)
{
  char *devSection = ltr_int_get_key("Global", "Input");
  if(devSection == NULL){
    return false;
  }
  char *devName = ltr_int_get_key(devSection, "Capture-device");
  char *devId = ltr_int_get_key(devSection, "Capture-device-id");
  if((devName == NULL) || (devId == NULL)){
    return false;
  }
  
  QString dn = devName;
  if(dn.compare((char *)"Webcam", Qt::CaseInsensitive) == 0){
    devType = WEBCAM;
  }else if(dn.compare((char *)"Webcam-face", Qt::CaseInsensitive) == 0){
	  devType = WEBCAM_FT;
  }else if(dn.compare((char *)"MacWebcam", Qt::CaseInsensitive) == 0){
	  devType = MACWEBCAM;
  }else if(dn.compare((char *)"MacWebcam-face", Qt::CaseInsensitive) == 0){
	  devType = MACWEBCAM_FT;
  }else if(dn.compare((char *)"Wiimote", Qt::CaseInsensitive) == 0){
    devType = WIIMOTE;
  }else if(dn.compare((char *)"Tir", Qt::CaseInsensitive) == 0){
    devType = TIR;
  }else if(dn.compare((char *)"Tir_openusb", Qt::CaseInsensitive) == 0){
    devType = TIR;
  }else{
    devType = NONE;
  }
  id = QString(devId);
  secName = QString(devSection);
  return true;
}

bool PrefProxy::getActiveDevice(deviceType_t &devType, QString &id)
{
  QString tmp;
  return getActiveDevice(devType, id, tmp);
}

bool PrefProxy::getActiveModel(QString &model)
{
  char *modelSection = ltr_int_get_key("Global", "Model");
  if(modelSection == NULL){
    return false;
  }
  model = modelSection;
  return true;
}

bool PrefProxy::getModelList(QStringList &list)
{
  std::vector<std::string> sections;
  ltr_int_find_sections("Model-type", (void*)&sections);
  list.clear();
  for(size_t i = 0; i < sections.size(); ++i){
    list.append(QString::fromStdString(sections[i]));
  }
  return (list.size() != 0);
}

bool PrefProxy::getProfiles(QStringList &list)
{
  std::vector<std::string> profiles;
  char *title;
  ltr_int_find_sections("Title", (void*)&profiles);
  list.clear();
  for(size_t i = 0; i < profiles.size(); ++i){
    title = ltr_int_get_key(profiles[i].c_str(), "Title");
    if(title != NULL){
      list.append(title);
    }
  }
  return (list.size() != 0);
}

bool PrefProxy::getProfileSection(const QString &name, QString &section)
{
  char *secName = ltr_int_find_section("Title", qPrintable(name));
  if(secName != NULL){
    section = secName;
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
  return QDir::homePath() + "/.config/linuxtrack/";
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

void PrefProxy::getSectionList(QStringList &list)
{
  std::vector<std::string> tmpList;
  ltr_int_get_section_list((void*)&tmpList);
  list.clear();
  for(size_t i = 0; i < tmpList.size(); ++i){
    list.append(QString::fromStdString(tmpList[i]));
  }
}



