#include <iostream>
#include "ltr_gui.h"


char webcam_item_name[] = "Webcam";
char wiimote_item_name[] = "Wiimote";
char tir_item_name[] = "TrackIR";
char tiro_item_name[] = "TrackIR (openusb)";


static QString& getFirstDeviceSection(const QString& device)
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


LinuxtrackGui::LinuxtrackGui(QWidget *parent): QWidget(parent)
{
  ui.setupUi(this);
  if(!open_pref((char *)"Global", (char *)"Input", &dev_selector)){
    //No input device....
    //!!!
  }
  wcp = NULL;
  ui.DeviceSelector->clear();
  ui.DeviceSelector->addItem(wiimote_item_name);
  ui.DeviceSelector->addItem(webcam_item_name);
  ui.DeviceSelector->addItem(tir_item_name);
  ui.DeviceSelector->addItem(tiro_item_name);
}


void LinuxtrackGui::on_DeviceSelector_currentIndexChanged(const QString &text)
{
  if(text == webcam_item_name){
    ui.DeviceSetupStack->setCurrentIndex(0);
    if(wcp == NULL){
      wcp = new WebcamPrefs(ui);
    }
    wcp->Activate();
  }else if(text == wiimote_item_name){
    ui.DeviceSetupStack->setCurrentIndex(1);
    //WiimotePrefsInit();
    QString &sec = getFirstDeviceSection("Wiimote");
    if(sec != ""){
      set_str(&dev_selector, sec.toAscii().data());
    }
  }else if(text == tir_item_name){
    QString &sec = getFirstDeviceSection("Tir");
    if(sec != ""){
      set_str(&dev_selector, sec.toAscii().data());
    }
  }else if(text == tiro_item_name){
    QString &sec = getFirstDeviceSection("Tir_openusb");
    if(sec != ""){
      set_str(&dev_selector, sec.toAscii().data());
    }else{
      std::cout<<"No such cestion!\n";
    }
  }
}


void LinuxtrackGui::on_QuitButton_pressed()
{
  close();
}