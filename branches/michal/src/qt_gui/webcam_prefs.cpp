#include <QMessageBox>
#include <iostream>
#include <QByteArray>
#include "ltr_gui.h"
#include "webcam_info.h"
#include "ltr_gui_prefs.h"
#include "pref_int.h"
#include "ltr_gui_prefs.h"

static QString currentId = QString("None");

void WebcamPrefs::Connect()
{
  QObject::connect(gui.WebcamFormats, SIGNAL(currentIndexChanged(const QString &)),
    this, SLOT(on_WebcamFormats_currentIndexChanged(const QString &)));
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
  if(!open_pref((char *)"Global", (char *)"Input", &dev_selector)){
    //No input device....
    //!!!
  }
}

WebcamPrefs::~WebcamPrefs()
{
  close_pref(&dev_selector);
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

void WebcamPrefs::on_WebcamFormats_currentIndexChanged(const QString &text)
{
  gui.WebcamResolutions->clear();
  if(currentId == "None"){
    return;
  }
  gui.WebcamResolutions->addItems(wc_info->getResolutions(gui.WebcamFormats->currentIndex()));
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

/*
void WebcamPrefs::on_RefreshWebcams_pressed()
{
  gui.WebcamIDs->clear();
  gui.WebcamIDs->addItem("None");
  QStringList &webcams = WebcamInfo::EnumerateWebcams();
  gui.WebcamIDs->addItems(webcams);
  delete(&webcams);
}
*/

void WebcamPrefs::Activate(const QString &ID)
{
  QString &sec = getFirstDeviceSection("Webcam");
  if(sec != ""){
    set_str(&dev_selector, sec.toAscii().data());
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
  }
}

void WebcamPrefs::on_WebcamThreshold_valueChanged(int i)
{
  std::cout<<"Threshold: "<<i<<std::endl;
}

void WebcamPrefs::on_WebcamMinBlob_valueChanged(int i)
{
  std::cout<<"Min Blob: "<<i<<std::endl;
}

void WebcamPrefs::on_WebcamMaxBlob_valueChanged(int i)
{
  std::cout<<"Max Blob: "<<i<<std::endl;
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

