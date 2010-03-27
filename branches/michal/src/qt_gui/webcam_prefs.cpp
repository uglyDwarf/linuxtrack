#include <QMessageBox>
#include <iostream>
#include <QByteArray>
#include "ltr_gui.h"
#include "webcam_prefs.h"
#include "webcam_info.h"
#include "ltr_gui_prefs.h"
#include "pref_int.h"
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
  QString res, fps;
  if(PREF.getKeyVal(currentSection, (char *)"Resolution", res) &&
    PREF.getKeyVal(currentSection, (char *)"Fps", fps)){
    res_index = wc_info->findRes(res, fps, wc_info->getFourcc(index));
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
    PREF.setKeyVal(currentSection, (char *)"Pixel-format", fmt);
    PREF.setKeyVal(currentSection, (char *)"Resolution", res);
    PREF.setKeyVal(currentSection, (char *)"Fps", fps);
  }
}

void WebcamPrefs::Activate(const QString &ID)
{
  QString sec;
  if(PREF.getFirstDeviceSection(QString("Webcam"), ID, sec)){
    PREF.activateDevice(sec);
    currentSection = sec;
  }else{
    sec = "Webcam";
    if(PREF.createDevice(sec)){
      PREF.addKeyVal(sec, (char *)"Capture-device", (char *)"Webcam");
      PREF.addKeyVal(sec, (char *)"Capture-device-id", ID);
      PREF.addKeyVal(sec, (char *)"Pixel-format", (char *)"");
      PREF.addKeyVal(sec, (char *)"Resolution", (char *)"");
      PREF.addKeyVal(sec, (char *)"Fps", (char *)"");
      PREF.addKeyVal(sec, (char *)"Threshold", QString::number(140));
      PREF.addKeyVal(sec, (char *)"Min-blob", QString::number(4));
      PREF.addKeyVal(sec, (char *)"Max-blob", QString::number(230));
      PREF.activateDevice(sec);
      currentSection = sec;
    }else{
      return;
    }
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
    QString fourcc, thres, bmin, bmax, res, fps;
    int fmt_index = 0;
    if(PREF.getKeyVal(sec, (char *)"Pixel-format", fourcc)){
      fmt_index = wc_info->findFourcc(fourcc);
      gui.WebcamFormats->setCurrentIndex(fmt_index);
    }
    on_WebcamFormats_activated(fmt_index);
    
    if(PREF.getKeyVal(sec, (char *)"Threshold", thres)){
      gui.WebcamThreshold->setValue(thres.toInt());
    }
    if(PREF.getKeyVal(sec, (char *)"Max-blob", bmax)){
      gui.WebcamMaxBlob->setValue(bmax.toInt());
    }
    if(PREF.getKeyVal(sec, (char *)"Min-blob", bmin)){
      gui.WebcamMinBlob->setValue(bmin.toInt());
    }
    
  }
}

void WebcamPrefs::on_WebcamThreshold_valueChanged(int i)
{
  std::cout<<"Threshold: "<<i<<std::endl;
  PREF.setKeyVal(currentSection, (char *)"Threshold", i);
}

void WebcamPrefs::on_WebcamMinBlob_valueChanged(int i)
{
  std::cout<<"Min Blob: "<<i<<std::endl;
  PREF.setKeyVal(currentSection, (char *)"Min-blob", i);
}

void WebcamPrefs::on_WebcamMaxBlob_valueChanged(int i)
{
  std::cout<<"Max Blob: "<<i<<std::endl;
  PREF.setKeyVal(currentSection, (char *)"Max-blob", i);
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

