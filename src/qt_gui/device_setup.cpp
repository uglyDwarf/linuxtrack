#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include "device_setup.h"
#ifdef DARWIN
#include "macwebcam_prefs.h"
#include "macwebcamft_prefs.h"
#else
#include "webcam_prefs.h"
#include "webcam_ft_prefs.h"
#include "joy_prefs.h"
#endif
#include "tir_prefs.h"
#include "wiimote_prefs.h"
#include "help_view.h"
#include "ltr_gui_prefs.h"
#include "guardian.h"
#include "tracking.h"
#include <iostream>


/* Coding:
            bit0 (lsb) - invert camera X values
            bit1       - invert camera Y values
            bit2       - switch X and Y values (applied first!)
            bit4       - invert pitch, roll, X and Z translations (for tracking from behind)
*/

QString DeviceSetup::descs[8] = {
    QString::fromUtf8("Normal"),                        //0
    QString::fromUtf8("Top to the right"),              //6
    QString::fromUtf8("Upside-down"),                   //3
    QString::fromUtf8("Top to the left"),               //5
    QString::fromUtf8("Normal, from behind"),           //8
    QString::fromUtf8("Top to the right, from behind"), //14
    QString::fromUtf8("Upside-down, from behind"),      //11
    QString::fromUtf8("Top to the left, from behind")   //12
  };

//int DeviceSetup::orientValues[] = {0, 6, 3, 5, 8, 14, 11, 13};
int DeviceSetup::orientValues[] = {ORIENT_NOP, // 0
                                   ORIENT_FLIP_Y | ORIENT_XCHG_XY, // 6
                                   ORIENT_FLIP_X | ORIENT_FLIP_Y, // 3
                                   ORIENT_FLIP_X | ORIENT_XCHG_XY, //5
                                   ORIENT_FROM_BEHIND, // 8
                                   ORIENT_FLIP_Y | ORIENT_XCHG_XY | ORIENT_FROM_BEHIND, // 14
                                   ORIENT_FLIP_X | ORIENT_FLIP_Y  | ORIENT_FROM_BEHIND, // 11
                                   ORIENT_FLIP_X | ORIENT_XCHG_XY | ORIENT_FROM_BEHIND}; // 13

DeviceSetup::DeviceSetup(Guardian *grd, QBoxLayout *tgt, QWidget *parent)
  : QWidget(parent), devPrefs(NULL), target(tgt)
{
  grd->regTgt(this);
  ui.setupUi(this);
  on_RefreshDevices_pressed();
  initOrientations();
}

DeviceSetup::~DeviceSetup()
{
  if(devPrefs != NULL){
    target->removeWidget(devPrefs);
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
  if(PREF.getKeyVal(QString::fromUtf8("Global"), QString::fromUtf8("Camera-orientation"), orient)){
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
  if(index < 0){
    return;
  }
  if(devPrefs != NULL){
    target->removeWidget(devPrefs);
    delete devPrefs;
    devPrefs = NULL;
  }
  QVariant v = ui.DeviceSelector->itemData(index);
  PrefsLink pl = v.value<PrefsLink>();
#ifndef DARWIN
  if(pl.deviceType == WEBCAM){
    devPrefs = new WebcamPrefs(pl.ID, this);
    emit deviceTypeChanged(pl.deviceType, QString::fromUtf8("Webcam"));
  }else
  if(pl.deviceType == WEBCAM_FT){
    devPrefs = new WebcamFtPrefs(pl.ID, this);
    emit deviceTypeChanged(pl.deviceType, QString::fromUtf8("Webcam Face Tracker"));
  }else
  if(pl.deviceType == JOYSTICK){
    devPrefs = new JoyPrefs(pl.ID, this);
    emit deviceTypeChanged(pl.deviceType, QString::fromUtf8("Joystick"));
  }else
#else
  if(pl.deviceType == MACWEBCAM){
    devPrefs = new MacWebcamPrefs(pl.ID, this);
    emit deviceTypeChanged(pl.deviceType, QString::fromUtf8("Webcam"));
  }else
  if(pl.deviceType == MACWEBCAM_FT){
    devPrefs = new MacWebcamFtPrefs(pl.ID, this);
    emit deviceTypeChanged(pl.deviceType, QString::fromUtf8("Webcam Face Tracker"));
  }else
#endif
  if(pl.deviceType == WIIMOTE){
    devPrefs = new WiimotePrefs(pl.ID, this);
    emit deviceTypeChanged(pl.deviceType, QString::fromUtf8("Wiimote"));
  }else
  if(pl.deviceType == TIR){
    devPrefs = new TirPrefs(pl.ID, this);
    emit deviceTypeChanged(pl.deviceType, QString::fromUtf8("TrackIR"));
  }
  if(devPrefs != NULL){
    target->insertWidget(-1, devPrefs);
  }
}

void DeviceSetup::on_CameraOrientation_activated(int index)
{
  if(index < 0){
    return;
  }
  PREF.setKeyVal(QString::fromUtf8("Global"), QString::fromUtf8("Camera-orientation"), orientValues[index]);
}

void DeviceSetup::on_RefreshDevices_pressed()
{
  refresh();
}


void DeviceSetup::refresh()
{
  ui.DeviceSelector->clear();
  bool res = false;
  res |= WiimotePrefs::AddAvailableDevices(*(ui.DeviceSelector));
  res |= TirPrefs::AddAvailableDevices(*(ui.DeviceSelector));
#ifdef DARWIN
  res |= MacWebcamFtPrefs::AddAvailableDevices(*(ui.DeviceSelector));
  res |= MacWebcamPrefs::AddAvailableDevices(*(ui.DeviceSelector));
#else
  res |= WebcamFtPrefs::AddAvailableDevices(*(ui.DeviceSelector));
  res |= WebcamPrefs::AddAvailableDevices(*(ui.DeviceSelector));
  res |= JoyPrefs::AddAvailableDevices(*(ui.DeviceSelector));
#endif
  if(!res){
    initialized = true;
  }
  on_DeviceSelector_activated(ui.DeviceSelector->currentIndex());
  initialized = true;
}




