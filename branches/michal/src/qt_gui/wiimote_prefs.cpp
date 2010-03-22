#include "wiimote_prefs.h"
#include "ltr_gui_prefs.h"

WiimotePrefs::WiimotePrefs(const Ui::LinuxtrackMainForm &ui) : gui(ui)
{
  Connect();
  if(!open_pref((char *)"Global", (char *)"Input", &dev_selector)){
    //No input device....
    //!!!
  }
}

WiimotePrefs::~WiimotePrefs()
{
  close_pref(&dev_selector);
}

void WiimotePrefs::Activate(const QString &ID)
{
  QString &sec = getFirstDeviceSection("Wiimote");
  if(sec != ""){
    set_str(&dev_selector, sec.toAscii().data());
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
