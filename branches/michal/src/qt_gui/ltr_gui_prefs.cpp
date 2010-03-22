#include "pref_int.h"
#include "ltr_gui_prefs.h"


QString& getFirstDeviceSection(const QString& device)
{
    char **sections = NULL;
    get_section_list(&sections);
    char *name;
    int i = 0;
    while((name = sections[i]) != NULL){
      char *dev_name;
      if((dev_name = get_key(name, (char *)"Capture-device")) != NULL){
	if(QString(dev_name) == device){
	  break;
	}
      }
      ++i;
    }
    QString *res;
    if(name != NULL){
      res = new QString(name);
    }else{
      res = new QString("");
    }
    array_cleanup(&sections);
    return *res;
}

