#include <QMessageBox>
#include <iostream>
#include <QByteArray>
#include "ltr_gui.h"
#include "webcam_prefs.h"
#include "wc_driver_prefs.h"
#include "webcam_info.h"
#include "ltr_gui_prefs.h"

static QString currentId = QString("None");

WebcamPrefs::WebcamPrefs(const QString &dev_id, QWidget *parent) : QWidget(parent), id(dev_id)
{
  ui.setupUi(this);
  Activate(id, true);
}

WebcamPrefs::~WebcamPrefs()
{
  ltr_int_wc_close_prefs();
}


static WebcamInfo *wc_info = NULL;

void WebcamPrefs::on_WebcamFormats_activated(int index)
{
  ui.WebcamResolutions->clear();
  if(currentId == "None"){
    return;
  }
  ui.WebcamResolutions->addItems(wc_info->getResolutions(index));
  int res_index = 0;
  int res_x, res_y;
  int fps_num, fps_den;
  if(ltr_int_wc_get_resolution(&res_x, &res_y) &&
    ltr_int_wc_get_fps(&fps_num, &fps_den)){
    res_index = wc_info->findRes(res_x, res_y, fps_num, fps_den, 
				 wc_info->getFourcc(index));
    ui.WebcamResolutions->setCurrentIndex(res_index);
  }
  on_WebcamResolutions_activated(res_index);
}

void WebcamPrefs::on_WebcamResolutions_activated(int index)
{
  if(ui.WebcamFormats->currentIndex() == -1){
    return;
  }
  QString res, fps, fmt;
  if(wc_info->findFmtSpecs(ui.WebcamFormats->currentIndex(), 
                           index, res, fps, fmt)){
    int x,y, num, den;
    WebcamInfo::decodeRes(res, x, y);
    WebcamInfo::decodeFps(fps, num, den);
    if(!initializing){
      ltr_int_wc_set_pixfmt(fmt.toAscii().data());
      ltr_int_wc_set_resolution(x, y);
      ltr_int_wc_set_fps(num, den);
    }
  }
}

bool WebcamPrefs::Activate(const QString &ID, bool init)
{
  bool res = false;
  QString sec;
  initializing = init;
  if(PREF.getFirstDeviceSection(QString("Webcam"), ID, sec)){
    QString currentDev, currentSection;
    deviceType_t devType;
    if(!PREF.getActiveDevice(devType, currentDev, currentSection) || (sec !=currentSection)){
      PREF.activateDevice(sec);
    }
  }else{
    sec = "Webcam";
    initializing = false;
    if(PREF.createSection(sec)){
      PREF.addKeyVal(sec, (char *)"Capture-device", (char *)"Webcam");
      PREF.addKeyVal(sec, (char *)"Capture-device-id", ID);
      PREF.addKeyVal(sec, (char *)"Pixel-format", (char *)"");
      PREF.addKeyVal(sec, (char *)"Resolution", (char *)"");
      PREF.addKeyVal(sec, (char *)"Fps", (char *)"");
      PREF.addKeyVal(sec, (char *)"Threshold", QString::number(140));
      PREF.addKeyVal(sec, (char *)"Min-blob", QString::number(4));
      PREF.addKeyVal(sec, (char *)"Max-blob", QString::number(230));
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
  ui.WebcamFormats->clear();
  ui.WebcamResolutions->clear();
  if((currentId != "None") && (currentId.size() != 0)){
    if(wc_info != NULL){
      delete(wc_info);
    }
    wc_info = new WebcamInfo(currentId);
    
    ui.WebcamFormats->addItems(wc_info->getFormats());
    QString fourcc, thres, bmin, bmax, res, fps, flip;
    int fmt_index = 0;
    const char *tmp = ltr_int_wc_get_pixfmt();
    //std::cout<<"4CC: "<<tmp<<std::endl;
    if(tmp != NULL){
      fourcc = tmp;
      fmt_index = wc_info->findFourcc(fourcc);
      ui.WebcamFormats->setCurrentIndex(fmt_index);
    }
    on_WebcamFormats_activated(fmt_index);
    
    ui.WebcamThreshold->setValue(ltr_int_wc_get_threshold());
    ui.WebcamMaxBlob->setValue(ltr_int_wc_get_max_blob());
    ui.WebcamMinBlob->setValue(ltr_int_wc_get_min_blob());
  }
  initializing = false;
  return res;
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

bool WebcamPrefs::AddAvailableDevices(QComboBox &combo)
{
  bool res = false;
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
      res = true;
    }
  }
  delete(&webcams);
  return res;
}

