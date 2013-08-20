#include <QMessageBox>
#include <iostream>
#include <QByteArray>
#include "ui_m_wc_setup.h"
#include "macwebcam_prefs.h"
#include "macwebcam_info.h"
#include "ltr_gui_prefs.h"
#include "wc_driver_prefs.h"

static QString currentId = QString("None");

/*
void MacWebcamPrefs::Connect()
{
  QObject::connect(ui.WebcamResolutionsMac, SIGNAL(activated(int)),
    this, SLOT(on_WebcamResolutions_activated(int)));
  QObject::connect(ui.WebcamThresholdMac, SIGNAL(valueChanged(int)),
    this, SLOT(on_WebcamThreshold_valueChanged(int)));
  QObject::connect(ui.WebcamMinBlobMac, SIGNAL(valueChanged(int)),
    this, SLOT(on_WebcamMinBlob_valueChanged(int)));
  QObject::connect(ui.WebcamMaxBlobMac, SIGNAL(valueChanged(int)),
    this, SLOT(on_WebcamMaxBlob_valueChanged(int)));
  QObject::connect(ui.FlipWebcamMac, SIGNAL(stateChanged(int)),
    this, SLOT(on_FlipWebcam_stateChanged(int)));
}
*/

MacWebcamPrefs::MacWebcamPrefs(const QString &dev_id, QWidget *parent) : QWidget(parent), id(dev_id)
{
  ui.setupUi(this);
  Activate(id, true);
}

MacWebcamPrefs::~MacWebcamPrefs()
{
  ltr_int_wc_close_prefs();
}

static MacWebcamInfo *wc_info = NULL;

void MacWebcamPrefs::on_WebcamResolutionsMac_activated(int index)
{
  (void) index;
  QString res;
  res = ui.WebcamResolutionsMac->currentText();
  
  int x,y;
  MacWebcamInfo::decodeRes(res, x, y);
  if(!initializing) ltr_int_wc_set_resolution(x, y);
}

bool MacWebcamPrefs::Activate(const QString &ID, bool init)
{
  QString sec;
  initializing = init;
  if(PREF.getFirstDeviceSection(QString("MacWebcam"), ID, sec)){
    QString currentDev, currentSection;
    deviceType_t devType;
    if(!PREF.getActiveDevice(devType, currentDev, currentSection) || (sec !=currentSection)){
      PREF.activateDevice(sec);
    }
  }else{
    sec = "MacWebcam";
    initializing = false;
    if(PREF.createSection(sec)){
      PREF.addKeyVal(sec, (char *)"Capture-device", (char *)"MacWebcam");
      PREF.addKeyVal(sec, (char *)"Capture-device-id", ID);
      PREF.addKeyVal(sec, (char *)"Resolution", (char *)"");
      PREF.addKeyVal(sec, (char *)"Threshold", QString::number(130));
      PREF.addKeyVal(sec, (char *)"Min-blob", QString::number(9));
      PREF.addKeyVal(sec, (char *)"Max-blob", QString::number(231));
      PREF.addKeyVal(sec, (char *)"Upside-down", (char *)"No");
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
  if((currentId != "None") && (currentId.size() != 0)){
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
    
    QString thres, bmin, bmax, flip;
    ui.WebcamThresholdMac->setValue(ltr_int_wc_get_threshold());
    ui.WebcamMaxBlobMac->setValue(ltr_int_wc_get_max_blob());
    ui.WebcamMinBlobMac->setValue(ltr_int_wc_get_min_blob());
  }
  initializing = false;
  return true;
}

void MacWebcamPrefs::on_WebcamThresholdMac_valueChanged(int i)
{
  if(!initializing) ltr_int_wc_set_threshold(i);
}

void MacWebcamPrefs::on_WebcamMinBlobMac_valueChanged(int i)
{
  if(!initializing) ltr_int_wc_set_min_blob(i);
}

void MacWebcamPrefs::on_WebcamMaxBlobMac_valueChanged(int i)
{
  if(!initializing) ltr_int_wc_set_max_blob(i);
}

bool MacWebcamPrefs::AddAvailableDevices(QComboBox &combo)
{
  bool res = false;
  QString id;
  deviceType_t dt;
  bool webcam_selected = false;
  if(PREF.getActiveDevice(dt,id) && (dt == MACWEBCAM)){
    webcam_selected = true;
  }
  QStringList &webcams = MacWebcamInfo::EnumerateWebcams();
  QStringList::iterator i;
  PrefsLink *pl;
  QVariant v;
  for(i = webcams.begin(); i != webcams.end(); ++i){
    pl = new PrefsLink(MACWEBCAM, *i);
    v.setValue(*pl);
    combo.addItem(*i, v);
    if(webcam_selected && (*i == id)){
      combo.setCurrentIndex(combo.count() - 1);
      res = true;
    }
  }
  delete(&webcams);
  return res;
}

