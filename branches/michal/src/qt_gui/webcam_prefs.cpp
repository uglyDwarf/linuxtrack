#include <QMessageBox>
#include <iostream>
#include <QByteArray>
#include "ltr_gui.h"
#include "webcam_info.h"
#include "pref_int.h"



static QString& getFirstDeviceSection(const QString& device)
{
    char **sections = NULL;
    get_section_list(&sections);
    char *name;
    int i = 0;
    while((name = sections[i]) != NULL){
      char *dev_name;
      if((dev_name = get_key(name, (char *)"Capture-device")) != NULL){
	if(QString(dev_name) == device){
	  break;
	}
      }
      ++i;
    }
    QString *res;
    if(name != NULL){
      res = new QString(name);
    }else{
      res = new QString("");
    }
    array_cleanup(&sections);
    return *res;
}


void WebcamPrefs::Connect()
{
  QObject::connect(gui.WebcamIDs, SIGNAL(currentIndexChanged(const QString &)),
    this, SLOT(on_WebcamIDs_currentIndexChanged(const QString &)));
  QObject::connect(gui.WebcamFormats, SIGNAL(currentIndexChanged(const QString &)),
    this, SLOT(on_WebcamFormats_currentIndexChanged(const QString &)));
  QObject::connect(gui.WebcamResolutions, SIGNAL(currentIndexChanged(const QString &)),
    this, SLOT(on_WebcamResolutions_currentIndexChanged(const QString &)));
  QObject::connect(gui.RefreshWebcams, SIGNAL(pressed()),
    this, SLOT(on_RefreshWebcams_pressed()));
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
  on_RefreshWebcams_pressed();
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

void WebcamPrefs::on_WebcamIDs_currentIndexChanged(const QString &text)
{
  gui.WebcamFormats->clear();
  gui.WebcamResolutions->clear();
  if((text != "None") && (text.size() != 0)){
    if(wc_info != NULL){
      delete(wc_info);
    }
    wc_info = new WebcamInfo(text);
    
    gui.WebcamFormats->addItems(wc_info->getFormats());
  }
}

void WebcamPrefs::on_WebcamFormats_currentIndexChanged(const QString &text)
{
  gui.WebcamResolutions->clear();
  QString id = gui.WebcamIDs->currentText();
  if(id == "None"){
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
    std::cout<<"Id: '" << gui.WebcamIDs->currentText().toAscii().data() 
             << "'" << std::endl;
    std::cout<<"Chci >" << fourcc.toAscii().data() << w << " x " << h << 
               " @ " << fps <<std::endl;
  }
}

void WebcamPrefs::on_RefreshWebcams_pressed()
{
  gui.WebcamIDs->clear();
  gui.WebcamIDs->addItem("None");
  QStringList &webcams = WebcamInfo::EnumerateWebcams();
  gui.WebcamIDs->addItems(webcams);
  delete(&webcams);
}

void WebcamPrefs::Activate()
{
  QString &sec = getFirstDeviceSection("Webcam");
  if(sec != ""){
    set_str(&dev_selector, sec.toAscii().data());
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

