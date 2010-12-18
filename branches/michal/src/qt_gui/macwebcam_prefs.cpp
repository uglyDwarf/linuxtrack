#include <QMessageBox>
#include <iostream>
#include <QByteArray>
#include "ltr_gui.h"
#include "macwebcam_prefs.h"
#include "macwebcam_info.h"
#include "ltr_gui_prefs.h"
#include "wc_driver_prefs.h"

static QString currentId = QString("None");
static QString currentSection = QString();

void WebcamPrefs::Connect()
{
  QObject::connect(gui.WebcamResolutionsMac, SIGNAL(activated(int)),
    this, SLOT(on_WebcamResolutions_activated(int)));
  QObject::connect(gui.WebcamThresholdMac, SIGNAL(valueChanged(int)),
    this, SLOT(on_WebcamThreshold_valueChanged(int)));
  QObject::connect(gui.WebcamMinBlobMac, SIGNAL(valueChanged(int)),
    this, SLOT(on_WebcamMinBlob_valueChanged(int)));
  QObject::connect(gui.WebcamMaxBlobMac, SIGNAL(valueChanged(int)),
    this, SLOT(on_WebcamMaxBlob_valueChanged(int)));
  QObject::connect(gui.FlipWebcamMac, SIGNAL(stateChanged(int)),
    this, SLOT(on_FlipWebcam_stateChanged(int)));
}

WebcamPrefs::WebcamPrefs(const Ui::LinuxtrackMainForm &ui) : gui(ui)
{
  Connect();
}

WebcamPrefs::~WebcamPrefs()
{
}

WebcamInfo *wc_info = NULL;

void WebcamPrefs::on_WebcamResolutions_activated(int index)
{
  (void) index;
  QString res;
  res = gui.WebcamResolutionsMac->currentText();
  
  int x,y;
  WebcamInfo::decodeRes(res, x, y);
  if(!initializing) ltr_int_wc_set_resolution(x, y);
}

void WebcamPrefs::Activate(const QString &ID, bool init)
{
  QString sec;
  initializing = init;
  if(PREF.getFirstDeviceSection(QString("Webcam"), ID, sec)){
    if(!initializing) PREF.activateDevice(sec);
    currentSection = sec;
  }else{
    sec = "Webcam";
    if(PREF.createSection(sec)){
      PREF.addKeyVal(sec, (char *)"Capture-device", (char *)"Webcam");
      PREF.addKeyVal(sec, (char *)"Capture-device-id", ID);
      PREF.addKeyVal(sec, (char *)"Resolution", (char *)"");
      PREF.addKeyVal(sec, (char *)"Threshold", QString::number(130));
      PREF.addKeyVal(sec, (char *)"Min-blob", QString::number(9));
      PREF.addKeyVal(sec, (char *)"Max-blob", QString::number(231));
      PREF.addKeyVal(sec, (char *)"Upside-down", (char *)"No");
      PREF.activateDevice(sec);
      currentSection = sec;
    }else{
      initializing = false;
      return;
    }
  }
  if(!ltr_int_wc_init_prefs()){
      initializing = false;
    return;
  }
  currentId = ID;
  gui.WebcamResolutionsMac->clear();
  if((currentId != "None") && (currentId.size() != 0)){
    if(wc_info != NULL){
      delete(wc_info);
    }
    wc_info = new WebcamInfo(currentId);
    gui.WebcamResolutionsMac->clear();
    gui.WebcamResolutionsMac->addItems(wc_info->getResolutions());
    int res_index = 0;
    int res_x, res_y;
    if(ltr_int_wc_get_resolution(&res_x, &res_y)){
      res_index = wc_info->findRes(res_x, res_y);
      gui.WebcamResolutionsMac->setCurrentIndex(res_index);
    }
    on_WebcamResolutions_activated(res_index);
    
    QString thres, bmin, bmax, flip;
    gui.WebcamThresholdMac->setValue(ltr_int_wc_get_threshold());
    gui.WebcamMaxBlobMac->setValue(ltr_int_wc_get_max_blob());
    gui.WebcamMinBlobMac->setValue(ltr_int_wc_get_min_blob());
    
    Qt::CheckState state = (ltr_int_wc_get_flip()) ? 
                       Qt::Checked : Qt::Unchecked;
    gui.FlipWebcamMac->setCheckState(state);
  }
  initializing = false;
}

void WebcamPrefs::on_WebcamThreshold_valueChanged(int i)
{
  if(!initializing) ltr_int_wc_set_threshold(i);
}

void WebcamPrefs::on_WebcamMinBlob_valueChanged(int i)
{
  if(!initializing) ltr_int_wc_set_min_blob(i);
}

void WebcamPrefs::on_WebcamMaxBlob_valueChanged(int i)
{
  if(!initializing) ltr_int_wc_set_max_blob(i);
}

void WebcamPrefs::on_FlipWebcam_stateChanged(int state)
{
  if(!initializing) ltr_int_wc_set_flip(state == Qt::Checked);
}

void WebcamPrefs::AddAvailableDevices(QComboBox &combo)
{
  QString id;
  deviceType_t dt;
  bool webcam_selected = false;
  if(PREF.getActiveDevice(dt,id) && (dt == WEBCAM)){
    webcam_selected = true;
  }
  QStringList &webcams = WebcamInfo::EnumerateWebcams();
  QStringList::iterator i;
  PrefsLink *pl;
  QVariant v;
  for(i = webcams.begin(); i != webcams.end(); ++i){
    pl = new PrefsLink(WEBCAM, *i);
    v.setValue(*pl);
    combo.addItem(*i, v);
    if(webcam_selected && (*i == id)){
      combo.setCurrentIndex(combo.count() - 1);
    }
  }
  delete(&webcams);
}

