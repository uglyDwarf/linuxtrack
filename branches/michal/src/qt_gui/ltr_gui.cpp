#include <iostream>
#include "ltr_gui.h"
#include "ltr_gui_prefs.h"
#include "prefs_link.h"

LinuxtrackGui::LinuxtrackGui(QWidget *parent) : QWidget(parent)
{
  ui.setupUi(this);
  wcp = new WebcamPrefs(ui);
  wiip = new WiimotePrefs(ui);
  tirp = new TirPrefs(ui);
  me = new ModelEdit(ui);
  sc = new ScpForm();
  helper = new LtrDevHelp(sc);
  on_RefreshDevices_pressed();
  showWindow.show();
  helper->show();
  
  
}

LinuxtrackGui::~LinuxtrackGui()
{
  delete wcp;
  delete wiip;
  delete tirp;
  delete me;
  delete sc;
  delete helper;
}

void LinuxtrackGui::closeEvent(QCloseEvent *event)
{
  showWindow.close();
  helper->close();
  sc->close();
  event->accept();
}

void LinuxtrackGui::on_DeviceSelector_activated(int index)
{
  if(index < 0){
    return;
  }
  QVariant v = ui.DeviceSelector->itemData(index);
  PrefsLink pl = v.value<PrefsLink>();
  if(pl.deviceType == WEBCAM){
    ui.DeviceSetupStack->setCurrentIndex(0);
    wcp->Activate(pl.ID);
  }else if(pl.deviceType == WIIMOTE){
    ui.DeviceSetupStack->setCurrentIndex(1);
    wiip->Activate(pl.ID);
  }else if(pl.deviceType == TIR){
    ui.DeviceSetupStack->setCurrentIndex(2);
    tirp->Activate(pl.ID);
  }
}

void LinuxtrackGui::on_RefreshDevices_pressed()
{
  ui.DeviceSelector->clear();
  WebcamPrefs::AddAvailableDevices(*(ui.DeviceSelector));
  WiimotePrefs::AddAvailableDevices(*(ui.DeviceSelector));
  TirPrefs::AddAvailableDevices(*(ui.DeviceSelector));
  on_DeviceSelector_activated(ui.DeviceSelector->currentIndex());
}

void LinuxtrackGui::on_QuitButton_pressed()
{
  close();
}

void LinuxtrackGui::on_EditSCButton_pressed()
{
  sc->show();
}