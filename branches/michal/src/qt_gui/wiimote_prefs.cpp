#include "wiimote_prefs.h"
#include "ltr_gui_prefs.h"
#include <iostream>

typedef enum{
  ON_LED1,  ON_LED2,  ON_LED3,  ON_LED4,
  OFF_LED1, OFF_LED2, OFF_LED3, OFF_LED4
} wii_leds_t;

static QString currentSection = QString();

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
    currentSection = sec;
  }else{
    sec = "Wiimote";
    if(PREF.createDevice(sec)){
      PREF.addKeyVal(sec, (char *)"Capture-device", (char *)"Wiimote");
      PREF.addKeyVal(sec, (char *)"Capture-device-id", ID);
      PREF.addKeyVal(sec, (char *)"Running-indication", (char *)"0100");
      PREF.addKeyVal(sec, (char *)"Paused-indication", (char *)"0010");
      PREF.activateDevice(sec);
      currentSection = sec;
    }
  }
  QString indication;
  if(PREF.getKeyVal(currentSection, (char *)"Running-indication", indication)){
    if(indication.size() == 4){
      if(indication[0] == QChar('1')) gui.Wii_r1->setCheckState(Qt::Checked);
      if(indication[1] == QChar('1')) gui.Wii_r2->setCheckState(Qt::Checked);
      if(indication[2] == QChar('1')) gui.Wii_r3->setCheckState(Qt::Checked);
      if(indication[3] == QChar('1')) gui.Wii_r4->setCheckState(Qt::Checked);
    }
  }  
  if(PREF.getKeyVal(currentSection, (char *)"Paused-indication", indication)){
    if(indication.size() == 4){
      if(indication[0] == QChar('1')) gui.Wii_p1->setCheckState(Qt::Checked);
      if(indication[1] == QChar('1')) gui.Wii_p2->setCheckState(Qt::Checked);
      if(indication[2] == QChar('1')) gui.Wii_p3->setCheckState(Qt::Checked);
      if(indication[3] == QChar('1')) gui.Wii_p4->setCheckState(Qt::Checked);
    }
  }  
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

static bool setIndication(int running, int index, bool state)
{
  QString indication;
  if(running == 1){
    if(PREF.getKeyVal(currentSection, (char *)"Running-indication", indication)){
      indication.replace(index-1, 1, state ? "1" : "0");
      return PREF.setKeyVal(currentSection, (char *)"Running-indication", indication);
    }
  }else{
    if(PREF.getKeyVal(currentSection, (char *)"Paused-indication", indication)){
      indication.replace(index-1, 1, state ? "1" : "0");
      return PREF.setKeyVal(currentSection, (char *)"Paused-indication", indication);
    }
  }
  return false;
}

void WiimotePrefs::indicationButtonStateChanged(int state)
{
  QObject *sender = QObject::sender();
  if(sender == 0){
    return;
  }
  bool bstate = (state == Qt::Unchecked) ? false : true;
  QString name = sender->objectName();
  if(name == "Wii_r1"){
    setIndication(1, 1, bstate);
  }else if(name == "Wii_r2"){
    setIndication(1, 2, bstate);
  }else if(name == "Wii_r3"){
    setIndication(1, 3, bstate);
  }else if(name == "Wii_r4"){
    setIndication(1, 4, bstate);
  }else if(name == "Wii_p1"){
    setIndication(0, 1, bstate);
  }else if(name == "Wii_p2"){
    setIndication(0, 2, bstate);
  }else if(name == "Wii_p3"){
    setIndication(0, 3, bstate);
  }else if(name == "Wii_p4"){
    setIndication(0, 4, bstate);
  }
}

void WiimotePrefs::Connect()
{
  QObject::connect(gui.Wii_r1, SIGNAL(stateChanged(int)),
    this, SLOT(indicationButtonStateChanged(int)));
  QObject::connect(gui.Wii_r2, SIGNAL(stateChanged(int)),
    this, SLOT(indicationButtonStateChanged(int)));
  QObject::connect(gui.Wii_r3, SIGNAL(stateChanged(int)),
    this, SLOT(indicationButtonStateChanged(int)));
  QObject::connect(gui.Wii_r4, SIGNAL(stateChanged(int)),
    this, SLOT(indicationButtonStateChanged(int)));
  QObject::connect(gui.Wii_p1, SIGNAL(stateChanged(int)),
    this, SLOT(indicationButtonStateChanged(int)));
  QObject::connect(gui.Wii_p2, SIGNAL(stateChanged(int)),
    this, SLOT(indicationButtonStateChanged(int)));
  QObject::connect(gui.Wii_p3, SIGNAL(stateChanged(int)),
    this, SLOT(indicationButtonStateChanged(int)));
  QObject::connect(gui.Wii_p4, SIGNAL(stateChanged(int)),
    this, SLOT(indicationButtonStateChanged(int)));
}

