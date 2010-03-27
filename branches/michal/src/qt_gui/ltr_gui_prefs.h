#ifndef LTR_GUI_PREFS__H
#define LTR_GUI_PREFS__H

#include "prefs_link.h"
#include <QString>

#define PREF PrefProxy::Pref()

class PrefProxy{
 private:
  PrefProxy();
  ~PrefProxy();
  static PrefProxy *prf;
 public:
  static PrefProxy& Pref();
  bool activateDevice(const QString &sectionName);
  bool getActiveDevice(deviceType_t &devType, QString &id);
  bool getKeyVal(const QString &sectionName, const QString &keyName, 
		 QString &result);
  bool setKeyVal(const QString &sectionName, const QString &keyName, 
		 const QString &value);
  bool setKeyVal(const QString &sectionName, const QString &keyName, 
                 const int &value);
  bool setKeyVal(const QString &sectionName, const QString &keyName, 
                 const float &value);
  bool getFirstDeviceSection(const QString &devType, QString &result);
  bool getFirstDeviceSection(const QString &devType, 
			     const QString &devId, QString &result);
  bool createDevice(QString &sectionName);
  bool addKeyVal(const QString &sectionName, const QString &keyName, 
		 const QString &value);
};



#endif