#include "wiimote_prefs.h"
#include "ltr_gui_prefs.h"

WiimotePrefs::WiimotePrefs(const Ui::LinuxtrackMainForm &ui) : gui(ui)
{
  Connect();
}

WiimotePrefs::~WiimotePrefs()
{
}

void WiimotePrefs::Activate(const QString &ID)
{
  QString sec;
  if(PREF.getFirstDeviceSection(QString("Wiimote"), sec)){
    PREF.activateDevice(sec);
  }else{
    //!!! default!!!
  }
}

void WiimotePrefs::AddAvailableDevices(QComboBox &combo)
{
  PrefsLink *pl = new PrefsLink(WIIMOTE, (char *)"Wiimote");
  QVariant v;
  v.setValue(*pl);
  combo.addItem((char *)"Wiimote", v);
}

void WiimotePrefs::Connect()
{
}
