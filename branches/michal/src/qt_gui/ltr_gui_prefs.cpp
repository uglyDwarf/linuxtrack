#include "pref_int.h"
#include "ltr_gui_prefs.h"
#include "map"

#include <QStringList>
#include <iostream>

PrefProxy *PrefProxy::prf = NULL;


PrefProxy::PrefProxy()
{
  if(!ltr_int_read_prefs(NULL, false)){
    ltr_int_log_message("Couldn't load preferences!\n");
    ltr_int_new_prefs();
    QString global = "Global";
    if(!createSection(global)){
      ltr_int_log_message("Can't create prefs at all... That is too bad.\n");
      exit(1);
    }
  }
}

PrefProxy::~PrefProxy()
{
  if(prf != NULL){
    delete prf;
  }
  ltr_int_free_prefs();
}

PrefProxy& PrefProxy::Pref()
{
  if(prf == NULL){
    prf = new PrefProxy();
  }
  return *prf;
}

bool PrefProxy::activateDevice(const QString &sectionName)
{
  pref_id dev_selector;
  if(!ltr_int_open_pref((char *)"Global", (char *)"Input", &dev_selector)){
    return addKeyVal((char *)"Global", (char *)"Input", sectionName);
  }
  ltr_int_set_str(&dev_selector, sectionName.toAscii().data());
  ltr_int_close_pref(&dev_selector);
  return true;
}

bool PrefProxy::activateModel(const QString &sectionName)
{
  pref_id dev_selector;
  if(!ltr_int_open_pref((char *)"Global", (char *)"Model", &dev_selector)){
    return addKeyVal((char *)"Global", (char *)"Model", sectionName);
  }
  ltr_int_set_str(&dev_selector, sectionName.toAscii().data());
  ltr_int_close_pref(&dev_selector);
  return true;
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
  char *val = ltr_int_get_key(sectionName.toAscii().data(), 
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
  char *val = ltr_int_get_key(NULL, keyName.toAscii().data());
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
  pref_id kv;
  if(!ltr_int_open_pref(sectionName.toAscii().data(), keyName.toAscii().data(), &kv)){
    return addKeyVal(sectionName, keyName, value);
  }
  bool res = true;
  if(!ltr_int_set_str(&kv, value.toAscii().data())){
    res = false;
  }
  ltr_int_close_pref(&kv);
  return res;
}

bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
                          const int &value)
{
  pref_id kv;
  if(!ltr_int_open_pref(sectionName.toAscii().data(), keyName.toAscii().data(), &kv)){
    return addKeyVal(sectionName, keyName, QString::number(value));
  }
  bool res = true;
  if(!ltr_int_set_int(&kv, value)){
    res = false;
  }
  ltr_int_close_pref(&kv);
  return res;
}

bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
                          const float &value)
{
  pref_id kv;
  if(!ltr_int_open_pref(sectionName.toAscii().data(), keyName.toAscii().data(), &kv)){
    return addKeyVal(sectionName, keyName, QString::number(value));
  }
  bool res = true;
  if(!ltr_int_set_flt(&kv, value)){
    res = false;
  }
  ltr_int_close_pref(&kv);
  return res;
}

bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
                          const double &value)
{
  pref_id kv;
  if(!ltr_int_open_pref(sectionName.toAscii().data(), keyName.toAscii().data(), &kv)){
    return addKeyVal(sectionName, keyName, QString::number(value));
  }
  bool res = true;
  if(!ltr_int_set_flt(&kv, (float)value)){
    res = false;
  }
  ltr_int_close_pref(&kv);
  return res;
}

bool PrefProxy::getFirstDeviceSection(const QString &devType, QString &result)
{
  char **sections = NULL;
  ltr_int_get_section_list(&sections);
  char *name;
  int i = 0;
  while((name = sections[i]) != NULL){
    char *dev_name;
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
    char *dev_name = ltr_int_get_key(name, (char *)"Capture-device");
    char *dev_id = ltr_int_get_key(name, (char *)"Capture-device-id");
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
  char *dev_section = ltr_int_get_key((char *)"Global", (char *)"Input");
  if(dev_section == NULL){
    return false;
  }
  char *dev_name = ltr_int_get_key(dev_section, (char *)"Capture-device");
  char *dev_id = ltr_int_get_key(dev_section, (char *)"Capture-device-id");
  if((dev_name == NULL) || (dev_id == NULL)){
    return false;
  }
  QString dn = dev_name;
  if(dn.compare((char *)"Webcam", Qt::CaseInsensitive) == 0){
    devType = WEBCAM;
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
  char *mod_section = ltr_int_get_key((char *)"Global", (char *)"Model");
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
    char *model_type = ltr_int_get_key(name, (char *)"Model-type");
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
    char *title = ltr_int_get_key(name, (char *)"Title");
    if(title != NULL){
      list.append(title);
    }
    ++i;
  }
  return (list.size() != 0);
}

bool PrefProxy::setCustomSection(const QString &name)
{
  return ltr_int_set_custom_section(name.toAscii().data());
}

bool PrefProxy::savePrefs()
{
  return ltr_int_save_prefs();
}

QString PrefProxy::getCustomSectionName()
{
  const char *sec = ltr_int_get_custom_section_name();
  if(sec == NULL){
    return QString("Default");
  }else{
    return QString(sec);
  }
}

