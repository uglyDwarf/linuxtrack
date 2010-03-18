
#include "ltr_gui.h"

LinuxtrackGui::LinuxtrackGui(QWidget *parent): QWidget(parent)
{
  ui.setupUi(this);
  WebcamPrefsInit();
  ui.DeviceSelector->clear();
  ui.DeviceSelector->addItem("Webcam");
  ui.DeviceSelector->addItem("Wiimote");
}

void LinuxtrackGui::on_DeviceSelector_currentIndexChanged(const QString &text)
{
  if(text == "Webcam"){
    ui.DeviceSetupStack->setCurrentIndex(0);
  }else if(text == "Wiimote"){
    ui.DeviceSetupStack->setCurrentIndex(1);
    //WiimotePrefsInit();
  }
}


void LinuxtrackGui::on_QuitButton_pressed()
{
  close();
}