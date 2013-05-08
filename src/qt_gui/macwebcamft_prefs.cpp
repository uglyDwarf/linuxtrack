#include <QMessageBox>
#include <iostream>
#include <QByteArray>
#include "ui_m_wcft_setup.h"
#include "macwebcamft_prefs.h"
#include "macwebcam_info.h"
#include "ltr_gui_prefs.h"
#include "wc_driver_prefs.h"
#include <utils.h>

static QString currentId = QString("None");

/*
void MacWebcamFtPrefs::Connect()
{
  QObject::connect(ui.WebcamFtResolutionsMac, SIGNAL(activated(int)),
    this, SLOT(on_WebcamResolutions_activated(int)));
  QObject::connect(ui.FindCascadeMac, SIGNAL(pressed()),
    this, SLOT(on_FindCascade_pressed()));
  QObject::connect(ui.CascadePathMac, SIGNAL(editingFinished()),
    this, SLOT(on_CascadePath_editingFinished()));
  QObject::connect(ui.ExpFilterFactorMac, SIGNAL(valueChanged(int)),
    this, SLOT(on_ExpFilterFactor_valueChanged(int)));
  QObject::connect(ui.OptimLevelMac, SIGNAL(valueChanged(int)),
    this, SLOT(on_OptimLevel_valueChanged(int)));
}
*/

MacWebcamFtPrefs::MacWebcamFtPrefs(const QString &dev_id, QWidget *parent) : QWidget(parent), id(dev_id)
{
//  Connect();
  ui.setupUi(this);
  prefInit = true;
  Activate(id, prefInit);
  prefInit = false;
}

MacWebcamFtPrefs::~MacWebcamFtPrefs()
{
  ltr_int_wc_close_prefs();
}

static MacWebcamInfo *wc_info = NULL;

void MacWebcamFtPrefs::on_WebcamFtResolutionsMac_activated(int index)
{
  (void) index;
  QString res;
  res = ui.WebcamFtResolutionsMac->currentText();
  
  int x,y;
  MacWebcamInfo::decodeRes(res, x, y);
  if(!initializing) ltr_int_wc_set_resolution(x, y);
}

bool MacWebcamFtPrefs::Activate(const QString &ID, bool init)
{
  QString sec;
  initializing = init;
  if(PREF.getFirstDeviceSection(QString("MacWebcam-face"), ID, sec)){
    QString currentDev, currentSection;
    deviceType_t devType;
    if(!PREF.getActiveDevice(devType, currentDev, currentSection) || (sec !=currentSection)){
      PREF.activateDevice(sec);
    }
  }else{
    sec = "MacWebcam-face";
    initializing = false;
    if(PREF.createSection(sec)){
      PREF.addKeyVal(sec, (char *)"Capture-device", (char *)"MacWebcam-face");
      PREF.addKeyVal(sec, (char *)"Capture-device-id", ID);
      PREF.addKeyVal(sec, (char *)"Resolution", (char *)"");
      QString cascadePath = PrefProxy::getDataPath("haarcascade_frontalface_alt2.xml");
	  QFileInfo finf = QFileInfo(cascadePath);
	  PREF.addKeyVal(sec, (char *)"Cascade", qPrintable(finf.canonicalFilePath()));
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
  ui.WebcamFtResolutionsMac->clear();
  if((currentId != "None") && (currentId.size() != 0)){
    if(wc_info != NULL){
      delete(wc_info);
    }
    wc_info = new MacWebcamInfo(currentId);
    ui.WebcamFtResolutionsMac->clear();
    ui.WebcamFtResolutionsMac->addItems(wc_info->getResolutions());
    int res_index = 0;
    int res_x, res_y;
    if(ltr_int_wc_get_resolution(&res_x, &res_y)){
      res_index = wc_info->findRes(res_x, res_y);
      ui.WebcamFtResolutionsMac->setCurrentIndex(res_index);
    }
    on_WebcamFtResolutionsMac_activated(res_index);
  }
  prefInit = true;
  const char *cascade = ltr_int_wc_get_cascade();
  QString cascadePath;
  if(cascade == NULL){
    cascadePath = PrefProxy::getDataPath("haarcascade_frontalface_alt2.xml");
  }else{
    cascadePath = cascade;
  }
  ui.CascadePathMac->setText(cascadePath);
  int n = (2.0 / ltr_int_wc_get_eff()) - 2;
  ui.ExpFilterFactorMac->setValue(n);
  on_ExpFilterFactorMac_valueChanged(n);
  ui.OptimLevelMac->setValue(ltr_int_wc_get_optim_level());
  prefInit = false;
  ltr_int_wc_close_prefs();
  initializing = false;
  return true;
}

bool MacWebcamFtPrefs::AddAvailableDevices(QComboBox &combo)
{
  bool res = false;
  QString id;
  deviceType_t dt;
  bool webcam_selected = false;
  if(PREF.getActiveDevice(dt,id) && (dt == MACWEBCAM_FT)){
    webcam_selected = true;
  }
  QStringList &webcams = MacWebcamInfo::EnumerateWebcams();
  QStringList::iterator i;
  PrefsLink *pl;
  QVariant v;
  for(i = webcams.begin(); i != webcams.end(); ++i){
    pl = new PrefsLink(MACWEBCAM_FT, *i);
    v.setValue(*pl);
    combo.addItem((*i)+" face tracker", v);
    if(webcam_selected && (*i == id)){
      combo.setCurrentIndex(combo.count() - 1);
      res = true;
    }
  }
  delete(&webcams);
  return res;
}

void MacWebcamFtPrefs::on_FindCascadeMac_pressed()
{
  QString path = ui.CascadePathMac->text();
  if(path.isEmpty()){
    path = QString(ltr_int_get_data_path(""));
  }else{
    QDir tmp(path);
    path = tmp.filePath(path);
  }
  QString fileName = QFileDialog::getOpenFileName(NULL,
     "Find Harr/LBP cascade", path, "xml Files (*.xml)");
  ui.CascadePathMac->setText(fileName);
  on_CascadePathMac_editingFinished();
}

void MacWebcamFtPrefs::on_CascadePathMac_editingFinished()
{
  if(!prefInit){
    ltr_int_wc_set_cascade(ui.CascadePathMac->text().toAscii().data());
  }
}

void MacWebcamFtPrefs::on_ExpFilterFactorMac_valueChanged(int value)
{
  float a = 2 / (value + 2.0); //EWMA window size
  //ui.ExpFiltFactorValMac->setText(QString("%1").arg(a, 0, 'g', 2));
  if(!prefInit){
    ltr_int_wc_set_eff(a);
  }
}

void MacWebcamFtPrefs::on_OptimLevelMac_valueChanged(int value)
{
  if(!prefInit){
    ltr_int_wc_set_optim_level(value);
  }
}

