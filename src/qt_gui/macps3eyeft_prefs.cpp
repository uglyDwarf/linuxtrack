#include <QMessageBox>
#include <iostream>
#include <QByteArray>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include "ui_m_ps3eye_ft_setup.h"
#include "macps3eyeft_prefs.h"
#include "ltr_gui_prefs.h"
#include "ps3_prefs.h"
#include "ps3eye_driver.h"
#include "utils.h"
#include "dyn_load.h"

static QString currentId = QString::fromUtf8("None");


typedef int (*ltr_int_ps3eye_found_fun_t)(void);
static ltr_int_ps3eye_found_fun_t ltr_int_ps3eye_found_fun = NULL;
static lib_fun_def_t functions[] = {
  {(char *)"ltr_int_ps3eye_found", (void*) &ltr_int_ps3eye_found_fun},
  {NULL, NULL}
};


static bool find_p3e(void)
{
  void *libhandle = NULL;
  int res = 0;
  if((libhandle = ltr_int_load_library((char *)"libp3eft", functions)) != NULL){
    res = ltr_int_ps3eye_found_fun();
    ltr_int_unload_library(libhandle, functions);
  }
  return res;
}


MacP3eFtPrefs::MacP3eFtPrefs(const QString &dev_id, QWidget *parent) : QWidget(parent), id(dev_id)
{
  ui.setupUi(this);
  Activate(id, true);
}

MacP3eFtPrefs::~MacP3eFtPrefs()
{
  ltr_int_ps3_close_prefs();
}

bool MacP3eFtPrefs::Activate(const QString &ID, bool init)
{
  QString sec;
  initializing = init;
  if(PREF.getFirstDeviceSection(QString::fromUtf8("Ps3Eye-face"), ID, sec)){
    QString currentDev, currentSection;
    deviceType_t devType;
    if(!PREF.getActiveDevice(devType, currentDev, currentSection) || (sec !=currentSection)){
      PREF.activateDevice(sec);
    }
  }else{
    sec = QString::fromUtf8("Ps3Eye-face");
    initializing = false;
    if(PREF.createSection(sec)){
      PREF.addKeyVal(sec, QString::fromUtf8("Capture-device"), QString::fromUtf8("Ps3Eye-face"));
      PREF.addKeyVal(sec, QString::fromUtf8("Capture-device-id"), ID);
      PREF.addKeyVal(sec, QString::fromUtf8("Mode"), QString::number(0));
      PREF.addKeyVal(sec, QString::fromUtf8("Exposure"), QString::number(120));
      PREF.addKeyVal(sec, QString::fromUtf8("Gain"), QString::number(20));
      PREF.addKeyVal(sec, QString::fromUtf8("Brightness"), QString::number(0));
      PREF.addKeyVal(sec, QString::fromUtf8("Contrast"), QString::number(32));
      PREF.addKeyVal(sec, QString::fromUtf8("Sharpness"), QString::number(0));
      PREF.addKeyVal(sec, QString::fromUtf8("AutoGain"), QString::fromUtf8("0"));
      PREF.addKeyVal(sec, QString::fromUtf8("AWB"), QString::fromUtf8("0"));
      PREF.addKeyVal(sec, QString::fromUtf8("AutoExposure"), QString::fromUtf8("0"));
      PREF.addKeyVal(sec, QString::fromUtf8("PowerLine-Frequency"), QString::fromUtf8("0"));
      PREF.addKeyVal(sec, QString::fromUtf8("Fps"), QString::fromUtf8("60"));
      QString cascadePath = PrefProxy::getDataPath(
                              QString::fromUtf8("/haarcascades/haarcascade_frontalface_alt2.xml"));
	  QFileInfo finf = QFileInfo(cascadePath);
	  PREF.addKeyVal(sec, QString::fromUtf8("Cascade"), finf.canonicalFilePath());
      PREF.activateDevice(sec);
    }else{
      return false;
    }
  }
  if(!ltr_int_ps3_init_prefs()){
    initializing = false;
    return false;
  }
  currentId = ID;
  if((currentId != QString::fromUtf8("None")) && (currentId.size() != 0)){
    int res_index = (ltr_int_ps3_get_mode() << 8) + ltr_int_ps3_get_ctrl_val(e_FPS);
    int index = 0;
    switch(res_index){
      case 187:
        index = 0;
        break;
      case 150:
        index = 1;
        break;
      case 137:
        index = 2;
        break;
      case 120:
        index = 3;
        break;
      case 100:
        index = 4;
        break;
      case 75:
        index = 5;
        break;
      case 60:
        index = 6;
        break;
      case 50:
        index = 7;
        break;
      case 37:
        index = 8;
        break;
      case 30:
        index = 9;
        break;
      case 256 + 60:
        index = 10;
        break;
      case 256 + 50:
        index = 11;
        break;
      case 256 + 40:
        index = 12;
        break;
      case 256 + 30:
        index = 13;
        break;
      case 256 + 15:
        index = 14;
        break;
      default:
        index = 0;
        break;
    }
    ui.WebcamResolutionsMac->setCurrentIndex(index);

    ui.EXPOSURE->setValue(ltr_int_ps3_get_ctrl_val(e_EXPOSURE));
    ui.GAIN->setValue(ltr_int_ps3_get_ctrl_val(e_GAIN));
    ui.BRIGHTNESS->setValue(ltr_int_ps3_get_ctrl_val(e_BRIGHTNESS));
    ui.CONTRAST->setValue(ltr_int_ps3_get_ctrl_val(e_CONTRAST));
    ui.SHARPNESS->setValue(ltr_int_ps3_get_ctrl_val(e_SHARPNESS));
    ui.AGC->setCheckState((ltr_int_ps3_get_ctrl_val(e_AUTOGAIN)) ? Qt::Checked : Qt::Unchecked);
    ui.AWB->setCheckState((ltr_int_ps3_get_ctrl_val(e_AUTOWHITEBALANCE)) ? Qt::Checked : Qt::Unchecked);
    ui.AEX->setCheckState((ltr_int_ps3_get_ctrl_val(e_AUTOEXPOSURE)) ? Qt::Checked : Qt::Unchecked);
    ui.PLF50->setCheckState((ltr_int_ps3_get_ctrl_val(e_PLFREQ)) ? Qt::Checked : Qt::Unchecked);

    const char *cascade = ltr_int_wc_get_cascade();
    QString cascadePath;
    if((cascade == NULL) || (!QFile::exists(QString::fromUtf8(cascade)))){
      cascadePath = PrefProxy::getDataPath(
                      QString::fromUtf8("/haarcascades/haarcascade_frontalface_alt2.xml"));
      ltr_int_wc_set_cascade(cascadePath.toUtf8().constData());
    }else{
      cascadePath = QString::fromUtf8(cascade);
    }
    ui.CascadePathMac->setText(cascadePath);
    int n = (2.0 / ltr_int_wc_get_eff()) - 2;
    ui.ExpFilterFactorMac->setValue(n);
    on_ExpFilterFactorMac_valueChanged(n);
    n = ltr_int_wc_get_optim_level();
    ui.OptimLevelMac->setValue(n);
    on_OptimLevelMac_valueChanged(n);

  }
  initializing = false;
  return true;
}

void MacP3eFtPrefs::on_WebcamResolutionsMac_activated(int index)
{
  if(index < 10){
    ltr_int_ps3_set_mode(0);
  }else{
    ltr_int_ps3_set_mode(1);
  }
  int fps;
  switch(index){
    case 0:
      fps = 187;
      break;
    case 1:
      fps = 150;
      break;
    case 2:
      fps = 137;
      break;
    case 3:
      fps = 120;
      break;
    case 4:
      fps = 100;
      break;
    case 5:
      fps = 75;
      break;
    case 6:
      fps = 60;
      break;
    case 7:
      fps = 50;
      break;
    case 8:
      fps = 37;
      break;
    case 9:
      fps = 30;
      break;
    case 10:
      fps = 60;
      break;
    case 11:
      fps = 50;
      break;
    case 12:
      fps = 40;
      break;
    case 13:
      fps = 30;
      break;
    case 14:
      fps = 15;
      break;
    default:
      fps = 187;
      break;
  }
  ltr_int_ps3_set_ctrl_val(e_FPS, fps);
}

bool MacP3eFtPrefs::AddAvailableDevices(QComboBox &combo)
{
  bool res = false;
  QString id;
  deviceType_t dt;
  bool webcam_selected = false;
  if(PREF.getActiveDevice(dt,id) && (dt == MACPS3EYE_FT)){
    webcam_selected = true;
  }
  QVariant v;
  if(find_p3e()){
    PrefsLink *pl = new PrefsLink(MACPS3EYE_FT, QString::fromUtf8("PS3Eye-face"));
    v.setValue(*pl);
    combo.addItem(QString::fromUtf8("Ps3Eye face tracker"), v);
    if(webcam_selected){
      combo.setCurrentIndex(combo.count() - 1);
      res = true;
    }
  }
  return res;
}

void MacP3eFtPrefs::on_EXPOSURE_valueChanged(int i)
{
  if(!initializing){ltr_int_ps3_set_ctrl_val(e_EXPOSURE, i);}
}

void MacP3eFtPrefs::on_GAIN_valueChanged(int i)
{
  if(!initializing){ltr_int_ps3_set_ctrl_val(e_GAIN, i);}
}

void MacP3eFtPrefs::on_BRIGHTNESS_valueChanged(int i)
{
  if(!initializing){ltr_int_ps3_set_ctrl_val(e_BRIGHTNESS, i);}
}

void MacP3eFtPrefs::on_CONTRAST_valueChanged(int i)
{
  if(!initializing){ltr_int_ps3_set_ctrl_val(e_CONTRAST, i);}
}

void MacP3eFtPrefs::on_SHARPNESS_valueChanged(int i)
{
  if(!initializing){ltr_int_ps3_set_ctrl_val(e_SHARPNESS, i);}
}

void MacP3eFtPrefs::on_AGC_stateChanged(int state)
{
  if(!initializing){ltr_int_ps3_set_ctrl_val(e_AUTOGAIN, state == Qt::Checked);}
}

void MacP3eFtPrefs::on_AWB_stateChanged(int state)
{
  if(!initializing){ltr_int_ps3_set_ctrl_val(e_AUTOWHITEBALANCE, state == Qt::Checked);}
}

void MacP3eFtPrefs::on_AEX_stateChanged(int state)
{
  if(!initializing){ltr_int_ps3_set_ctrl_val(e_AUTOEXPOSURE, state == Qt::Checked);}
}

void MacP3eFtPrefs::on_PLF50_stateChanged(int state)
{
  if(!initializing){ltr_int_ps3_set_ctrl_val(e_PLFREQ, state == Qt::Checked);}
}

void MacP3eFtPrefs::on_FindCascadeMac_pressed()
{
  QString path = ui.CascadePathMac->text();
  if(path.isEmpty()){
    path = QString::fromUtf8(ltr_int_get_data_path(""));
  }else{
    QDir tmp(path);
    path = tmp.filePath(path);
  }
  QString fileName = QFileDialog::getOpenFileName(NULL,
     QString::fromUtf8("Find Harr/LBP cascade"), path, QString::fromUtf8("xml Files (*.xml)"));
  ui.CascadePathMac->setText(fileName);
  on_CascadePathMac_editingFinished();
}

void MacP3eFtPrefs::on_CascadePathMac_editingFinished()
{
  if(!initializing){
    ltr_int_wc_set_cascade(ui.CascadePathMac->text().toUtf8().constData());
  }
}

void MacP3eFtPrefs::on_ExpFilterFactorMac_valueChanged(int value)
{
  float a = 2 / (value + 2.0); //EWMA window size
  //ui.ExpFiltFactorValMac->setText(QString("%1").arg(a, 0, 'g', 2));
  if(!initializing){
    ltr_int_wc_set_eff(a);
  }
}

void MacP3eFtPrefs::on_OptimLevelMac_valueChanged(int value)
{
  if(!initializing){
    ltr_int_wc_set_optim_level(value);
  }
}

#include "moc_macps3eyeft_prefs.cpp"

