#include <QMessageBox>
#include <iostream>
#include <QByteArray>
#include "ltr_gui.h"
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
  QObject::connect(gui.WebcamResolutions, SIGNAL(currentIndexChanged(const QString &)),
    this, SLOT(on_WebcamResolutions_currentIndexChanged(const QString &)));
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

static bool res_n_fps2fields(const QString &str, int &w, int &h, float &fps)
{
  const QRegExp &rexp = QRegExp("^\\s*(\\d+)\\s*[xX]\\s*(\\d+)\\s*@\\s*(\\S+)\\s*$");
  if(rexp.indexIn(str) == -1){
    return false;
  }
  w = rexp.cap(1).toInt();
  h = rexp.cap(2).toInt();
  fps = rexp.cap(3).toFloat();
  return true;
}


WebcamInfo *wc_info = NULL;

void WebcamPrefs::on_WebcamFormats_activated(int index)
{
  gui.WebcamResolutions->clear();
  if(currentId == "None"){
    return;
  }
  PREF.setKeyVal(currentSection, (char *)"Pixel-format", wc_info->getFourcc(index));
  gui.WebcamResolutions->addItems(wc_info->getResolutions(index));
}

void WebcamPrefs::on_WebcamResolutions_currentIndexChanged(const QString &text)
{
  if(gui.WebcamFormats->currentIndex() == -1){
    return;
  }
  int w = 0;
  int h = 0;
  float fps = 0.0f;
  const QString &fourcc = wc_info->getFourcc(gui.WebcamFormats->currentIndex());
  if(res_n_fps2fields(text, w, h, fps)){
    std::cout<<"Id: '" << currentId.toAscii().data() 
             << "'" << std::endl;
    std::cout<<"Chci >" << fourcc.toAscii().data() << w << " x " << h << 
               " @ " << fps <<std::endl;
  }
}

void WebcamPrefs::Activate(const QString &ID)
{
  QString sec;
  if(PREF.getFirstDeviceSection(QString("Webcam"), ID, sec)){
    PREF.activateDevice(sec);
    currentSection = sec;
  }else{
    //!!! Create default!!!
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
    QString fourcc, thres, bmin, bmax;
    int index = 0;
    if(PREF.getKeyVal(sec, (char *)"Pixel-format", fourcc)){
      index = wc_info->findFourcc(fourcc);
      gui.WebcamFormats->setCurrentIndex(index);
    }
    on_WebcamFormats_activated(index);
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
  PREF.setKeyVal(currentSection, (char *)"Threshold", QString::number(i));
}

void WebcamPrefs::on_WebcamMinBlob_valueChanged(int i)
{
  std::cout<<"Min Blob: "<<i<<std::endl;
  PREF.setKeyVal(currentSection, (char *)"Min-blob", QString::number(i));
}

void WebcamPrefs::on_WebcamMaxBlob_valueChanged(int i)
{
  std::cout<<"Max Blob: "<<i<<std::endl;
  PREF.setKeyVal(currentSection, (char *)"Max-blob", QString::number(i));
}

void WebcamPrefs::AddAvailableDevices(QComboBox &combo)
{
  QStringList &webcams = WebcamInfo::EnumerateWebcams();
  QStringList::iterator i;
  PrefsLink *pl;
  QVariant v;
  for(i = webcams.begin(); i != webcams.end(); ++i){
    pl = new PrefsLink(WEBCAM, *i);
    v.setValue(*pl);
    combo.addItem(*i, v);
  }
  delete(&webcams);
}

