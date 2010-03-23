#include <iostream>
#include "ltr_gui.h"
#include "ltr_gui_prefs.h"
#include "prefs_link.h"

LinuxtrackGui::LinuxtrackGui(QWidget *parent): QWidget(parent)
{
  ui.setupUi(this);
  if(!open_pref((char *)"Global", (char *)"Input", &dev_selector)){
    //No input device....
    //!!!
  }
  wcp = NULL;
  wiip = NULL;
  on_RefreshDevices_pressed();
  showWindow.show();
  helper.show();
}


void LinuxtrackGui::on_DeviceSelector_currentIndexChanged(int index)
{
  if(index < 0){
    return;
  }
  QVariant v = ui.DeviceSelector->itemData(index);
  PrefsLink pl = v.value<PrefsLink>();
  if(pl.deviceType == WEBCAM){
    ui.DeviceSetupStack->setCurrentIndex(0);
    if(wcp == NULL){
      wcp = new WebcamPrefs(ui);
    }
    wcp->Activate(pl.ID);
  }else if(pl.deviceType == WIIMOTE){
    ui.DeviceSetupStack->setCurrentIndex(1);
    if(wiip == NULL){
      wiip = new WiimotePrefs(ui);
    }
    wiip->Activate(pl.ID);
/*  }else if(text == tir_item_name){
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
    }*/
  }
}

void LinuxtrackGui::on_RefreshDevices_pressed()
{
  ui.DeviceSelector->clear();
  WebcamPrefs::AddAvailableDevices(*(ui.DeviceSelector));
  WiimotePrefs::AddAvailableDevices(*(ui.DeviceSelector));
}

void LinuxtrackGui::on_QuitButton_pressed()
{
  showWindow.close();
  helper.close();
  close();
}