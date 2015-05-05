#include <QMessageBox>
#include <iostream>
#include <QByteArray>
#include "ui_m_ps3eye_setup.h"
#include "macps3eye_prefs.h"
#include "ltr_gui_prefs.h"
#include "wc_driver_prefs.h"
#include "macwebcam_info.h"

static QString currentId = QString::fromUtf8("None");

MacP3ePrefs::MacP3ePrefs(const QString &dev_id, QWidget *parent) : QWidget(parent), id(dev_id)
{
  ui.setupUi(this);
  Activate(id, true);
}

MacP3ePrefs::~MacP3ePrefs()
{
  ltr_int_wc_close_prefs();
}

static MacWebcamInfo *wc_info = NULL;

void MacP3ePrefs::on_WebcamResolutionsMac_activated(int index)
{
  (void) index;
  QString res;
  res = ui.WebcamResolutionsMac->currentText();

  int x,y;
  MacWebcamInfo::decodeRes(res, x, y);
  if(!initializing) ltr_int_wc_set_resolution(x, y);
}

bool MacP3ePrefs::Activate(const QString &ID, bool init)
{
  QString sec;
  initializing = init;
  if(PREF.getFirstDeviceSection(QString::fromUtf8("Ps3Eye"), ID, sec)){
    QString currentDev, currentSection;
    deviceType_t devType;
    if(!PREF.getActiveDevice(devType, currentDev, currentSection) || (sec !=currentSection)){
      PREF.activateDevice(sec);
    }
  }else{
    sec = QString::fromUtf8("Ps3Eye");
    initializing = false;
    if(PREF.createSection(sec)){
      PREF.addKeyVal(sec, QString::fromUtf8("Capture-device"), QString::fromUtf8("Ps3Eye"));
      PREF.addKeyVal(sec, QString::fromUtf8("Capture-device-id"), ID);
      PREF.addKeyVal(sec, QString::fromUtf8("Resolution"), QString::fromUtf8(""));
      PREF.addKeyVal(sec, QString::fromUtf8("Threshold"), QString::number(130));
      PREF.addKeyVal(sec, QString::fromUtf8("Min-blob"), QString::number(9));
      PREF.addKeyVal(sec, QString::fromUtf8("Max-blob"), QString::number(231));
      PREF.addKeyVal(sec, QString::fromUtf8("Exposure"), QString::number(120));
      PREF.addKeyVal(sec, QString::fromUtf8("Gain"), QString::number(20));
      PREF.addKeyVal(sec, QString::fromUtf8("Brightness"), QString::number(20));
      PREF.addKeyVal(sec, QString::fromUtf8("Contrast"), QString::number(37));
      PREF.addKeyVal(sec, QString::fromUtf8("Sharpness"), QString::number(0));
      PREF.addKeyVal(sec, QString::fromUtf8("AGC"), QString::fromUtf8("No"));
      PREF.addKeyVal(sec, QString::fromUtf8("AWB"), QString::fromUtf8("No"));
      PREF.activateDevice(sec);
    }else{
      return false;
    }
  }
  if(!ltr_int_wc_init_prefs()){
    initializing = false;
    return false;
  }
  currentId = ID;
  ui.WebcamResolutionsMac->clear();
  if((currentId != QString::fromUtf8("None")) && (currentId.size() != 0)){
    if(wc_info != NULL){
      delete(wc_info);
    }
    wc_info = new MacWebcamInfo(currentId);
    ui.WebcamResolutionsMac->addItems(wc_info->getResolutions());
    int res_index = 0;
    int res_x, res_y;
    if(ltr_int_wc_get_resolution(&res_x, &res_y)){
      res_index = wc_info->findRes(res_x, res_y);
      ui.WebcamResolutionsMac->setCurrentIndex(res_index);
    }
    on_WebcamResolutionsMac_activated(res_index);

    ui.WebcamThresholdMac->setValue(ltr_int_wc_get_threshold());
    ui.WebcamMaxBlobMac->setValue(ltr_int_wc_get_max_blob());
    ui.WebcamMinBlobMac->setValue(ltr_int_wc_get_min_blob());
    ui.EXPOSURE->setValue(ltr_int_wc_get_exposure());
    ui.GAIN->setValue(ltr_int_wc_get_gain());
    ui.BRIGHTNESS->setValue(ltr_int_wc_get_brightness());
    ui.CONTRAST->setValue(ltr_int_wc_get_contrast());
    ui.SHARPNESS->setValue(ltr_int_wc_get_sharpness());
    ui.AGC->setCheckState((ltr_int_wc_get_agc()) ? Qt::Checked : Qt::Unchecked);
    ui.AWB->setCheckState((ltr_int_wc_get_awb()) ? Qt::Checked : Qt::Unchecked);
  }
  initializing = false;
  return true;
}

void MacP3ePrefs::on_WebcamThresholdMac_valueChanged(int i)
{
  if(!initializing) ltr_int_wc_set_threshold(i);
}

void MacP3ePrefs::on_WebcamMinBlobMac_valueChanged(int i)
{
  if(!initializing) ltr_int_wc_set_min_blob(i);
}

void MacP3ePrefs::on_WebcamMaxBlobMac_valueChanged(int i)
{
  if(!initializing) ltr_int_wc_set_max_blob(i);
}

bool MacP3ePrefs::AddAvailableDevices(QComboBox &combo)
{
  bool res = false;
  QString id;
  deviceType_t dt;
  bool webcam_selected = false;
  if(PREF.getActiveDevice(dt,id) && (dt == MACPS3EYE)){
    webcam_selected = true;
  }
  PrefsLink *pl = new PrefsLink(MACPS3EYE, QString::fromUtf8("PS3Eye"));
  QVariant v;
  v.setValue(*pl);
  combo.addItem(QString::fromUtf8("Ps3Eye"), v);
  if(webcam_selected){
    combo.setCurrentIndex(combo.count() - 1);
    res = true;
  }
  return res;
}

void MacP3ePrefs::on_EXPOSURE_valueChanged(int i)
{
  if(!initializing){ltr_int_wc_set_exposure(i);}
}

void MacP3ePrefs::on_GAIN_valueChanged(int i)
{
  if(!initializing){ltr_int_wc_set_gain(i);}
}

void MacP3ePrefs::on_BRIGHTNESS_valueChanged(int i)
{
  if(!initializing){ltr_int_wc_set_brightness(i);}
}

void MacP3ePrefs::on_CONTRAST_valueChanged(int i)
{
  if(!initializing){ltr_int_wc_set_contrast(i);}
}

void MacP3ePrefs::on_SHARPNESS_valueChanged(int i)
{
  if(!initializing){ltr_int_wc_set_sharpness(i);}
}

void MacP3ePrefs::on_AGC_stateChanged(int state)
{
  if(!initializing){ltr_int_wc_set_agc(state == Qt::Checked);}
}

void MacP3ePrefs::on_AWB_stateChanged(int state)
{
  if(!initializing){ltr_int_wc_set_awb(state == Qt::Checked);}
}

