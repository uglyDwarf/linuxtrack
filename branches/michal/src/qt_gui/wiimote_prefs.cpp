#include "wiimote_prefs.h"
#include "ltr_gui_prefs.h"
#include "wii_driver_prefs.h"
#include <iostream>

typedef enum{
  ON_LED1,  ON_LED2,  ON_LED3,  ON_LED4,
  OFF_LED1, OFF_LED2, OFF_LED3, OFF_LED4
} wii_leds_t;

static QString currentSection = QString();

WiimotePrefs::WiimotePrefs(const Ui::LinuxtrackMainForm &ui) : gui(ui), initializing(false)
{
  PREF;
  Connect();
}

WiimotePrefs::~WiimotePrefs()
{
}

static void setCheckBox(QCheckBox *box, bool val)
{
  if(val){
    box->setCheckState(Qt::Checked);
  }else{
    box->setCheckState(Qt::Unchecked);
  }
}

void WiimotePrefs::Activate(const QString &ID, bool init)
{
  QString sec;
  initializing= init;
  if(PREF.getFirstDeviceSection(QString("Wiimote"), sec)){
    if(!initializing) PREF.activateDevice(sec);
    currentSection = sec;
  }else{
    sec = "Wiimote";
    if(PREF.createSection(sec)){
      PREF.addKeyVal(sec, (char *)"Capture-device", (char *)"Wiimote");
      PREF.addKeyVal(sec, (char *)"Capture-device-id", ID);
      PREF.addKeyVal(sec, (char *)"Running-indication", (char *)"0100");
      PREF.addKeyVal(sec, (char *)"Paused-indication", (char *)"0010");
      PREF.activateDevice(sec);
      currentSection = sec;
    }
  }
  ltr_int_wii_init_prefs();
  QString indication;
  bool d1, d2, d3, d4;
  if(ltr_int_get_run_indication(&d1, &d2, &d3, &d4)){
    setCheckBox(gui.Wii_r1, d1);
    setCheckBox(gui.Wii_r2, d2);
    setCheckBox(gui.Wii_r3, d3);
    setCheckBox(gui.Wii_r4, d4);
  }  
  if(ltr_int_get_pause_indication(&d1, &d2, &d3, &d4)){
    setCheckBox(gui.Wii_p1, d1);
    setCheckBox(gui.Wii_p2, d2);
    setCheckBox(gui.Wii_p3, d3);
    setCheckBox(gui.Wii_p4, d4);
  }
  initializing = false;
}

void WiimotePrefs::AddAvailableDevices(QComboBox &combo)
{
  QString id;
  deviceType_t dt;
  bool wiimote_selected = false;
  if(PREF.getActiveDevice(dt,id)){
    if(dt == WIIMOTE){
      wiimote_selected = true;
    }
  }
  
  PrefsLink *pl = new PrefsLink(WIIMOTE, (char *)"Wiimote");
  QVariant v;
  v.setValue(*pl);
  combo.addItem((char *)"Wiimote", v);
  if(wiimote_selected){
    combo.setCurrentIndex(combo.count() - 1);
  }
}

static bool getState(QCheckBox *b)
{
  if(b->isChecked()){
    return true;
  }else{
    return false;
  }
}

void WiimotePrefs::runIndicationChanged(int state)
{
  (void) state;
  if(!initializing)
    ltr_int_set_run_indication(getState(gui.Wii_r1), getState(gui.Wii_r2), getState(gui.Wii_r3), getState(gui.Wii_r4));
}

void WiimotePrefs::pauseIndicationChanged(int state)
{
  (void) state;
  if(!initializing)
    ltr_int_set_pause_indication(getState(gui.Wii_p1), getState(gui.Wii_p2), getState(gui.Wii_p3), getState(gui.Wii_p4));
}

void WiimotePrefs::Connect()
{
  QObject::connect(gui.Wii_r1, SIGNAL(stateChanged(int)),
    this, SLOT(runIndicationChanged(int)));
  QObject::connect(gui.Wii_r2, SIGNAL(stateChanged(int)),
    this, SLOT(runIndicationChanged(int)));
  QObject::connect(gui.Wii_r3, SIGNAL(stateChanged(int)),
    this, SLOT(runIndicationChanged(int)));
  QObject::connect(gui.Wii_r4, SIGNAL(stateChanged(int)),
    this, SLOT(runIndicationChanged(int)));
  QObject::connect(gui.Wii_p1, SIGNAL(stateChanged(int)),
    this, SLOT(pauseIndicationChanged(int)));
  QObject::connect(gui.Wii_p2, SIGNAL(stateChanged(int)),
    this, SLOT(pauseIndicationChanged(int)));
  QObject::connect(gui.Wii_p3, SIGNAL(stateChanged(int)),
    this, SLOT(pauseIndicationChanged(int)));
  QObject::connect(gui.Wii_p4, SIGNAL(stateChanged(int)),
    this, SLOT(pauseIndicationChanged(int)));
}

