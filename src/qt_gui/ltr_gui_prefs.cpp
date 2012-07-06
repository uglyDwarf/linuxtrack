#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include "pref_int.h"
#include "pref_global.h"
#include "ltr_gui_prefs.h"
#include "map"

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
  ltr_int_log_message("Couldn't load preferences!\n");
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
  if(getKeyVal("Global", "Prefix", prefix) && (prefix == appPath)){
    //Intentionaly left empty
  }else{
    prefix = appPath;
    bool res = true;
    res &= setKeyVal("Global", "Prefix", appPath);
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
  return ltr_int_change_key((char *)"Global", (char *)"Input", 
			    sectionName.toAscii().data());
}

bool PrefProxy::activateModel(const QString &sectionName)
{
  return ltr_int_change_key((char *)"Global", (char *)"Model", 
			    sectionName.toAscii().data());
}

bool PrefProxy::createSection(QString 
&sectionName)
{
  int i = 0;
  QString newSecName = sectionName;
  while(1){
    if(!ltr_int_section_exists(newSecName.toAscii().data())){
      break;
    }
    newSecName = QString("%1_%2").arg(sectionName).
                                           arg(QString::number(++i));
  }
  ltr_int_add_section(newSecName.toAscii().data());
  sectionName = newSecName;
  return true;
}

bool PrefProxy::getKeyVal(const QString &sectionName, const QString &keyName, 
			  QString &result)
{
  const char *val = ltr_int_get_key(sectionName.toAscii().data(), 
                      keyName.toAscii().data());
  if(val != NULL){
    result = val;
    return true;
  }else{
    return false;
  }
}

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

bool PrefProxy::addKeyVal(const QString &sectionName, const QString &keyName, 
			  const QString &value)
{
  return ltr_int_add_key(sectionName.toAscii().data(), keyName.toAscii().data(), 
		 value.toAscii().data());
}



bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
			  const QString &value)
{
  return ltr_int_change_key(sectionName.toAscii().data(), keyName.toAscii().data(),
			    value.toAscii().data());
}

bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
                          const int &value)
{
  return ltr_int_change_key_int(sectionName.toAscii().data(), keyName.toAscii().data(),
			    value);
}

bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
                          const float &value)
{
  return ltr_int_change_key_flt(sectionName.toAscii().data(), keyName.toAscii().data(),
			    value);
}

bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
                          const double &value)
{
  return ltr_int_change_key_flt(sectionName.toAscii().data(), keyName.toAscii().data(),
			    (float)value);
}

bool PrefProxy::getFirstDeviceSection(const QString &devType, QString &result)
{
  char **sections = NULL;
  ltr_int_get_section_list(&sections);
  char *name;
  int i = 0;
  while((name = sections[i]) != NULL){
    const char *dev_name;
    if((dev_name = ltr_int_get_key(name, (char *)"Capture-device")) != NULL){
      if(QString(dev_name) == devType){
	break;
      }
    }
    ++i;
  }
  bool res;
  if(name != NULL){
    result = QString(name);
    res = true;
  }else{
    res = false;
  }
  ltr_int_array_cleanup(&sections);
  return res;
}

bool PrefProxy::getFirstDeviceSection(const QString &devType, 
				      const QString &devId, QString &result)
{
  char **sections = NULL;
  ltr_int_get_section_list(&sections);
  char *name;
  int i = 0;
  while((name = sections[i]) != NULL){
    const char *dev_name = ltr_int_get_key(name, (char *)"Capture-device");
    const char *dev_id = ltr_int_get_key(name, (char *)"Capture-device-id");
    if((dev_name != NULL) && (dev_id != NULL)){
      if((QString(dev_name) == devType) && (QString(dev_id) == devId)){
	break;
      }
    }
    ++i;
  }
  bool res;
  if(name != NULL){
    result = QString(name);
    res = true;
  }else{
    res = false;
  }
  ltr_int_array_cleanup(&sections);
  return res;
}

bool PrefProxy::getActiveDevice(deviceType_t &devType, QString &id)
{
  const char *dev_section = ltr_int_get_key((char *)"Global", (char *)"Input");
  if(dev_section == NULL){
    return false;
  }
  const char *dev_name = ltr_int_get_key(dev_section, (char *)"Capture-device");
  const char *dev_id = ltr_int_get_key(dev_section, (char *)"Capture-device-id");
  if((dev_name == NULL) || (dev_id == NULL)){
    return false;
  }
  QString dn = dev_name;
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
  id = dev_id;
  return true;
}

bool PrefProxy::getActiveModel(QString &model)
{
  const char *mod_section = ltr_int_get_key((char *)"Global", (char *)"Model");
  if(mod_section == NULL){
    return false;
  }
  model = mod_section;
  return true;
}

bool PrefProxy::getModelList(QStringList &list)
{
  char **sections = NULL;
  list.clear();
  ltr_int_get_section_list(&sections);
  char *name;
  int i = 0;
  while((name = sections[i]) != NULL){
    const char *model_type = ltr_int_get_key(name, (char *)"Model-type");
    if(model_type != NULL){
      list.append(name);
    }
    ++i;
  }
  return (list.size() != 0);
}

bool PrefProxy::getProfiles(QStringList &list)
{
  char **sections = NULL;
  list.clear();
  ltr_int_get_section_list(&sections);
  char *name;
  int i = 0;
  while((name = sections[i]) != NULL){
    const char *title = ltr_int_get_key(name, (char *)"Title");
    if(title != NULL){
      list.append(title);
    }
    ++i;
  }
  return (list.size() != 0);
}

bool PrefProxy::setCustomSection(const QString &name)
{
  //std::cout<<"Setting custom section "<<name.toStdString()<<std::endl;
  bool res = ltr_int_set_custom_section(name.toStdString().c_str());
  /*
  if(res){
    std::cout<<"Custom section set OK!!!"<<std::endl;
  }else{
    std::cout<<"Custom section not set!!!"<<std::endl;
  }
  */
  return res;
}

bool PrefProxy::savePrefs()
{
  bool res = ltr_int_save_prefs();
  return res;
}

QString PrefProxy::getCustomSectionName()
{
  const char *sec = ltr_int_get_custom_section_name();
  //std::cout<<"Getting custom section name "<<sec<<std::endl;
  if(sec == NULL){
    return QString("Default");
  }else{
    return QString(sec);
  }
}

QString PrefProxy::getCustomSectionTitle()
{
  const char *sec = ltr_int_get_custom_section_name();
  if(sec == NULL){
    //std::cout<<"Getting custom section title Default (no section)"<<std::endl;
    return QString("Default");
  }else{
    QString title;
    if(getKeyVal("Title", title)){
      //std::cout<<"Getting custom section title"<<title.toStdString()<<std::endl;
      return title;
    }else{
      //std::cout<<"Getting custom section title Default (no title)"<<std::endl;
      return QString("Default");
    }
  }
}

QString PrefProxy::getDataPath(QString file)
{
  char *path = ltr_int_get_data_path_prefix(file.toAscii().data(), 
                                            QApplication::applicationDirPath().toAscii().data());
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
  char *path = ltr_int_get_lib_path(file.toAscii().data());
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
  ltr_int_close_prefs();
  ltr_int_new_prefs();
  ltr_int_read_prefs(NULL, true);
  checkPrefix(true);
  return true;
}

void PrefProxy::announceModelChange()
{
  ltr_int_announce_model_change();
}
