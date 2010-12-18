#include <QMessageBox>
#include <iostream>
#include <QByteArray>
#include "ltr_gui.h"
#include "webcam_prefs.h"
#include "wc_driver_prefs.h"
#include "webcam_info.h"
#include "ltr_gui_prefs.h"

static QString currentId = QString("None");
static QString currentSection = QString();

void WebcamPrefs::Connect()
{
  QObject::connect(gui.WebcamFormats, SIGNAL(activated(int)),
    this, SLOT(on_WebcamFormats_activated(int)));
  QObject::connect(gui.WebcamResolutions, SIGNAL(activated(int)),
    this, SLOT(on_WebcamResolutions_activated(int)));
  QObject::connect(gui.WebcamThreshold, SIGNAL(valueChanged(int)),
    this, SLOT(on_WebcamThreshold_valueChanged(int)));
  QObject::connect(gui.WebcamMinBlob, SIGNAL(valueChanged(int)),
    this, SLOT(on_WebcamMinBlob_valueChanged(int)));
  QObject::connect(gui.WebcamMaxBlob, SIGNAL(valueChanged(int)),
    this, SLOT(on_WebcamMaxBlob_valueChanged(int)));
  QObject::connect(gui.FlipWebcam, SIGNAL(stateChanged(int)),
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

void WebcamPrefs::on_WebcamFormats_activated(int index)
{
  gui.WebcamResolutions->clear();
  if(currentId == "None"){
    return;
  }
  gui.WebcamResolutions->addItems(wc_info->getResolutions(index));
  int res_index = 0;
  int res_x, res_y;
  int fps_num, fps_den;
  if(ltr_int_wc_get_resolution(&res_x, &res_y) &&
    ltr_int_wc_get_fps(&fps_num, &fps_den)){
    res_index = wc_info->findRes(res_x, res_y, fps_num, fps_den, 
				 wc_info->getFourcc(index));
    gui.WebcamResolutions->setCurrentIndex(res_index);
  }
  on_WebcamResolutions_activated(res_index);
}

void WebcamPrefs::on_WebcamResolutions_activated(int index)
{
  if(gui.WebcamFormats->currentIndex() == -1){
    return;
  }
  QString res, fps, fmt;
  if(wc_info->findFmtSpecs(gui.WebcamFormats->currentIndex(), 
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
      PREF.addKeyVal(sec, (char *)"Pixel-format", (char *)"");
      PREF.addKeyVal(sec, (char *)"Resolution", (char *)"");
      PREF.addKeyVal(sec, (char *)"Fps", (char *)"");
      PREF.addKeyVal(sec, (char *)"Threshold", QString::number(140));
      PREF.addKeyVal(sec, (char *)"Min-blob", QString::number(4));
      PREF.addKeyVal(sec, (char *)"Max-blob", QString::number(230));
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
  gui.WebcamFormats->clear();
  gui.WebcamResolutions->clear();
  if((currentId != "None") && (currentId.size() != 0)){
    if(wc_info != NULL){
      delete(wc_info);
    }
    wc_info = new WebcamInfo(currentId);
    
    
    gui.WebcamFormats->addItems(wc_info->getFormats());
    QString fourcc, thres, bmin, bmax, res, fps, flip;
    int fmt_index = 0;
    const char *tmp = ltr_int_wc_get_pixfmt();
    if(tmp != NULL){
      fourcc = tmp;
      fmt_index = wc_info->findFourcc(fourcc);
      gui.WebcamFormats->setCurrentIndex(fmt_index);
    }
    on_WebcamFormats_activated(fmt_index);
    
    gui.WebcamThreshold->setValue(ltr_int_wc_get_threshold());
    gui.WebcamMaxBlob->setValue(ltr_int_wc_get_max_blob());
    gui.WebcamMinBlob->setValue(ltr_int_wc_get_min_blob());
    
    Qt::CheckState state = (ltr_int_wc_get_flip()) ? 
                       Qt::Checked : Qt::Unchecked;
    gui.FlipWebcam->setCheckState(state);
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

