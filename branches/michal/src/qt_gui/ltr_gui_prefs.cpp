#include "pref_int.h"
#include "ltr_gui_prefs.h"

PrefProxy *PrefProxy::prf = NULL;


PrefProxy::PrefProxy()
{
  
  if(!read_prefs(NULL, false)){
    log_message("Couldn't load preferences!\n");
  }
}

PrefProxy::~PrefProxy()
{
  if(prf != NULL){
    delete prf;
  }
  free_prefs();
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
  if(!open_pref((char *)"Global", (char *)"Input", &dev_selector)){
    return false;
  }
  set_str(&dev_selector, sectionName.toAscii().data());
  close_pref(&dev_selector);
  return true;
}

bool PrefProxy::getKeyVal(const QString &sectionName, const QString &keyName, 
			  QString &result)
{
  char *val = get_key(sectionName.toAscii().data(), 
                      keyName.toAscii().data());
  if(val != NULL){
    result = val;
    return true;
  }else{
    return false;
  }
}

bool PrefProxy::setKeyVal(const QString &sectionName, const QString &keyName, 
			  const QString &value)
{
  pref_id kv;
  if(!open_pref(sectionName.toAscii().data(), keyName.toAscii().data(), &kv)){
    return false;
  }
  bool res = true;
  if(!set_str(&kv, value.toAscii().data())){
    res = false;
  }
  close_pref(&kv);
  return res;
}

bool PrefProxy::getFirstDeviceSection(const QString &devType, QString &result)
{
    char **sections = NULL;
    get_section_list(&sections);
    char *name;
    int i = 0;
    while((name = sections[i]) != NULL){
      char *dev_name;
      if((dev_name = get_key(name, (char *)"Capture-device")) != NULL){
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
    array_cleanup(&sections);
    return res;
}

bool PrefProxy::getFirstDeviceSection(const QString &devType, 
				      const QString &devId, QString &result)
{
    char **sections = NULL;
    get_section_list(&sections);
    char *name;
    int i = 0;
    while((name = sections[i]) != NULL){
      char *dev_name = get_key(name, (char *)"Capture-device");
      char *dev_id = get_key(name, (char *)"Capture-device-id");
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
    array_cleanup(&sections);
    return res;
}


