#include <QMessageBox>
#include <iostream>
#include <QByteArray>
#include "ui_l_wcft_setup.h"
#include "webcam_ft_prefs.h"
#include "wc_driver_prefs.h"
#include "webcam_info.h"
#include "ltr_gui_prefs.h"

static QString currentId = QString::fromUtf8("None");

WebcamFtPrefs::WebcamFtPrefs(const QString &dev_id, QWidget *parent) : QWidget(parent), id(dev_id)
{
  ui.setupUi(this);
  Activate(id, true);
}

WebcamFtPrefs::~WebcamFtPrefs()
{
  ltr_int_wc_close_prefs();
}


static WebcamInfo *wc_info = NULL;

void WebcamFtPrefs::on_WebcamFtFormats_activated(int index)
{
  ui.WebcamFtResolutions->clear();
  if(currentId == QString::fromUtf8("None")){
    std::cout<<"None!"<<std::endl;
    return;
  }
  ui.WebcamFtResolutions->addItems(wc_info->getResolutions(index));
  int res_index = 0;
  int res_x, res_y;
  int fps_num, fps_den;
  if(ltr_int_wc_get_resolution(&res_x, &res_y) &&
    ltr_int_wc_get_fps(&fps_num, &fps_den)){
    res_index = wc_info->findRes(res_x, res_y, fps_num, fps_den, 
				 wc_info->getFourcc(index));
    ui.WebcamFtResolutions->setCurrentIndex(res_index);
  }
  on_WebcamFtResolutions_activated(res_index);
}

void WebcamFtPrefs::on_WebcamFtResolutions_activated(int index)
{
  if(ui.WebcamFtFormats->currentIndex() == -1){
    return;
  }
  QString res, fps, fmt;
  if(wc_info->findFmtSpecs(ui.WebcamFtFormats->currentIndex(), 
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

bool WebcamFtPrefs::Activate(const QString &ID, bool init)
{
  bool res = false;
  QString sec;
  initializing = init;
  if(PREF.getFirstDeviceSection(QString::fromUtf8("Webcam-face"), ID, sec)){
    QString currentDev, currentSection;
    deviceType_t devType;
    if(!PREF.getActiveDevice(devType, currentDev, currentSection) || (sec !=currentSection)){
      PREF.activateDevice(sec);
    }
  }else{
    sec = QString::fromUtf8("Webcam-face");
    initializing = false;
    if(PREF.createSection(sec)){
      PREF.addKeyVal(sec, QString::fromUtf8("Capture-device"), QString::fromUtf8("Webcam-face"));
      PREF.addKeyVal(sec, QString::fromUtf8("Capture-device-id"), ID);
      PREF.addKeyVal(sec, QString::fromUtf8("Pixel-format"), QString::fromUtf8(""));
      PREF.addKeyVal(sec, QString::fromUtf8("Resolution"), QString::fromUtf8(""));
      PREF.addKeyVal(sec, QString::fromUtf8("Fps"), QString::fromUtf8(""));
      QString cascadePath = PrefProxy::getDataPath(QString::fromUtf8("haarcascade_frontalface_alt2.xml"));
      QFileInfo finf = QFileInfo(cascadePath);
      PREF.addKeyVal(sec, QString::fromUtf8("Cascade"), finf.canonicalFilePath());
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
  ui.WebcamFtFormats->clear();
  ui.WebcamFtResolutions->clear();
  if((currentId != QString::fromUtf8("None")) && (currentId.size() != 0)){
    if(wc_info != NULL){
      delete(wc_info);
    }
    wc_info = new WebcamInfo(currentId);
    
    
    ui.WebcamFtFormats->addItems(wc_info->getFormats());
    QString fourcc, thres, bmin, bmax, res, fps, flip;
    int fmt_index = 0;
    const char *tmp = ltr_int_wc_get_pixfmt();
    if(tmp != NULL){
      fourcc = QString::fromUtf8(tmp);
      fmt_index = wc_info->findFourcc(fourcc);
      ui.WebcamFtFormats->setCurrentIndex(fmt_index);
    }
    on_WebcamFtFormats_activated(fmt_index);
    const char *cascade = ltr_int_wc_get_cascade();
    QString cascadePath;
    if((cascade == NULL) || (!QFile::exists(QString::fromUtf8(cascade)))){
      cascadePath = PrefProxy::getDataPath(QString::fromUtf8("haarcascade_frontalface_alt2.xml"));
      ltr_int_wc_set_cascade(qPrintable(cascadePath));
    }else{
      cascadePath = QString::fromUtf8(cascade);
    }
    ui.CascadePath->setText(cascadePath);
    int n = (2.0 / ltr_int_wc_get_eff()) - 2;
    ui.ExpFilterFactor->setValue(n);
    on_ExpFilterFactor_valueChanged(n);
    ui.OptimLevel->setValue(ltr_int_wc_get_optim_level());
  }
  initializing = false;
  return res;
}

bool WebcamFtPrefs::AddAvailableDevices(QComboBox &combo)
{
  bool res = false;
  QString id;
  deviceType_t dt;
  bool webcam_selected = false;
  if(PREF.getActiveDevice(dt,id) && (dt == WEBCAM_FT)){
    webcam_selected = true;
  }
  
  QStringList &webcams = WebcamInfo::EnumerateWebcams();
  QStringList::iterator i;
  PrefsLink *pl;
  QVariant v;
  for(i = webcams.begin(); i != webcams.end(); ++i){
    pl = new PrefsLink(WEBCAM_FT, *i);
    v.setValue(*pl);
    combo.addItem((*i)+QString::fromUtf8(" face tracker"), v);
    if(webcam_selected && (*i == id)){
      combo.setCurrentIndex(combo.count() - 1);
      res = true;
    }
  }
  delete(&webcams);
  return res;
}



void WebcamFtPrefs::on_FindCascade_pressed()
{
  QString path = ui.CascadePath->text();
  if(path.isEmpty()){
    path = QString::fromUtf8("/");
  }else{
    QDir tmp(path);
    path = tmp.filePath(path);
  }
  QString fileName = QFileDialog::getOpenFileName(NULL,
     QString::fromUtf8("Find Harr/LBP cascade"), path, QString::fromUtf8("xml Files (*.xml)"));
  ui.CascadePath->setText(fileName);
  on_CascadePath_editingFinished();
}

void WebcamFtPrefs::on_CascadePath_editingFinished()
{
  if(!initializing){
    ltr_int_wc_set_cascade(ui.CascadePath->text().toAscii().data());
  }
}

void WebcamFtPrefs::on_ExpFilterFactor_valueChanged(int value)
{
  float a = 2 / (value + 2.0); //EWMA window size
//  ui.ExpFiltFactorVal->setText(QString("%1").arg(a, 0, 'g', 2));
  if(!initializing){
    ltr_int_wc_set_eff(a);
  }
}

void WebcamFtPrefs::on_OptimLevel_valueChanged(int value)
{
  if(!initializing){
    ltr_int_wc_set_optim_level(value);
  }
}



