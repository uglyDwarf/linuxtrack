#include "device_setup.h"
#include "webcam_prefs.h"
#include "webcam_ft_prefs.h"
#include "macwebcam_prefs.h"
#include "macwebcamft_prefs.h"
#include "tir_prefs.h"
#include "wiimote_prefs.h"
#include "help_view.h"
#include "ltr_gui_prefs.h"
#include <iostream>


/* Coding:
            bit0 (lsb) - invert camera X values
            bit1       - invert camera Y values
            bit2       - switch X and Y values (applied first!)
            bit4       - invert pitch, roll, X and Z translations (for tracking from behind)
*/

QString DeviceSetup::descs[8] = {
    "Normal",                        //0
    "Top to the right",              //6
    "Upside-down",                   //3
    "Top to the left",               //5
    "Normal, from behind",           //8
    "Top to the right, from behind", //14
    "Upside-down, from behind",      //11
    "Top to the left, from behind"   //12
  };

int DeviceSetup::orientValues[] = {0, 6, 3, 5, 8, 14, 11, 13};

DeviceSetup::DeviceSetup(QWidget *parent) : QWidget(parent), devPrefs(NULL)
{
  ui.setupUi(this);
  on_RefreshDevices_pressed();
  initOrientations();
}

DeviceSetup::~DeviceSetup()
{
  if(devPrefs != NULL){
    ui.DeviceSetupSite->removeWidget(devPrefs);
    delete devPrefs;
    devPrefs = NULL;
  }
}

void DeviceSetup::initOrientations()
{
  int i;
  int orientVal = 0;
  int orientIndex = 0;
  
  QString orient;
  if(PREF.getKeyVal("Global", "Camera-orientation", orient)){
    orientVal=orient.toInt();
  }

  //Initialize Orientations combobox and lookup saved val
  ui.CameraOrientation->clear();
  for(i = 0; i < 8; ++i){
    ui.CameraOrientation->addItem(descs[i]);
    if(orientValues[i] == orientVal){
      orientIndex = i;
    }
  }
  
  ui.CameraOrientation->setCurrentIndex(orientIndex);
}

void DeviceSetup::on_DeviceSelector_activated(int index)
{
  std::cout<<"Index:"<<index<<std::endl;
  if(index < 0){
    return;
  }
  if(devPrefs != NULL){
    ui.DeviceSetupSite->removeWidget(devPrefs);
    delete devPrefs;
    devPrefs = NULL;
  }
  QVariant v = ui.DeviceSelector->itemData(index);
  PrefsLink pl = v.value<PrefsLink>();
  if(pl.deviceType == WEBCAM){
    devPrefs = new WebcamPrefs(pl.ID, this);
  }else 
  if(pl.deviceType == WEBCAM_FT){
    devPrefs = new WebcamFtPrefs(pl.ID, this);
  }else 
  if(pl.deviceType == MACWEBCAM){
    devPrefs = new MacWebcamPrefs(pl.ID, this);
  }else 
  if(pl.deviceType == MACWEBCAM_FT){
    devPrefs = new MacWebcamFtPrefs(pl.ID, this);
  }else 
  if(pl.deviceType == WIIMOTE){
    devPrefs = new WiimotePrefs(pl.ID, this);
  }else 
  if(pl.deviceType == TIR){
    devPrefs = new TirPrefs(pl.ID, this);
  }
  if(devPrefs != NULL){
    ui.DeviceSetupSite->addWidget(devPrefs);
  }
}

void DeviceSetup::on_CameraOrientation_activated(int index)
{
  if(index < 0){
    return;
  }
  PREF.setKeyVal("Global", "Camera-orientation", orientValues[index]);
}

void DeviceSetup::on_RefreshDevices_pressed()
{
  refresh();
}


void DeviceSetup::refresh()
{
  ui.DeviceSelector->clear();
  bool res = false; 
  res |= WebcamPrefs::AddAvailableDevices(*(ui.DeviceSelector));
  res |= WebcamFtPrefs::AddAvailableDevices(*(ui.DeviceSelector));
  res |= WiimotePrefs::AddAvailableDevices(*(ui.DeviceSelector));
  res |= TirPrefs::AddAvailableDevices(*(ui.DeviceSelector));
  if(!res){
    initialized = true;
  }
  on_DeviceSelector_activated(ui.DeviceSelector->currentIndex());
  initialized = true;
}




