#include <iostream>
#include "ltr_gui.h"
#include "ltr_gui_prefs.h"
#include "pref_int.h"
#include "ltr_gui_prefs.h"
#include "tir_prefs.h"
#include "pathconfig.h"
#include "dyn_load.h"
#include <QFile>

static QString currentId = QString("None");
static QString currentSection = QString();
static int tirType = 0;

typedef int (*probe_tir_fun_t)();
static probe_tir_fun_t probe_tir_fun = NULL;
static lib_fun_def_t functions[] = {
  {(char *)"ltr_int_tir_found", (void*) &probe_tir_fun},
  {NULL, NULL}
};


static int probeTir()
{
  void *libhandle = NULL;
  int res = 0;
  if((libhandle = ltr_int_load_library((char *)"libtir.so", functions)) != NULL){
    res = probe_tir_fun();
    ltr_int_unload_library(libhandle, functions);
  }
  return res;
}


void TirPrefs::Connect()
{
  QObject::connect(gui.TirThreshold, SIGNAL(valueChanged(int)),
    this, SLOT(on_TirThreshold_valueChanged(int)));
  QObject::connect(gui.TirMinBlob, SIGNAL(valueChanged(int)),
    this, SLOT(on_TirMinBlob_valueChanged(int)));
  QObject::connect(gui.TirMaxBlob, SIGNAL(valueChanged(int)),
    this, SLOT(on_TirMaxBlob_valueChanged(int)));
  QObject::connect(gui.TirStatusBright, SIGNAL(valueChanged(int)),
    this, SLOT(on_TirStatusBright_valueChanged(int)));
  QObject::connect(gui.TirIrBright, SIGNAL(valueChanged(int)),
    this, SLOT(on_TirIrBright_valueChanged(int)));
  QObject::connect(gui.TirSignalizeStatus, SIGNAL(stateChanged(int)),
    this, SLOT(on_TirSignalizeStatus_stateChanged(int)));
}

TirPrefs::TirPrefs(const Ui::LinuxtrackMainForm &ui) : gui(ui)
{
  Connect();
}

TirPrefs::~TirPrefs()
{
}

static bool haveFirmware()
{
  if(QFile::exists(PrefProxy::getDataPath("tir4.fw.gz")) && 
     QFile::exists(PrefProxy::getDataPath("tir5.fw.gz"))){
    return true;
  }else{
    return false;
  }
}

void TirPrefs::Activate(const QString &ID)
{
  QString sec;
  if(PREF.getFirstDeviceSection(QString("Tir"), sec)){
    PREF.activateDevice(sec);
    currentSection = sec;
  }else{
    sec = "TrackIR";
    if(PREF.createSection(sec)){
      PREF.addKeyVal(sec, (char *)"Capture-device", (char *)"Tir");
      PREF.addKeyVal(sec, (char *)"Capture-device-id", ID);
      PREF.addKeyVal(sec, (char *)"Threshold", QString::number(140));
      PREF.addKeyVal(sec, (char *)"Min-blob", QString::number(4));
      PREF.addKeyVal(sec, (char *)"Max-blob", QString::number(230));
      PREF.addKeyVal(sec, (char *)"Status-led-brightness", QString::number(0));
      PREF.addKeyVal(sec, (char *)"Ir-led-brightness", QString::number(7));
      PREF.addKeyVal(sec, (char *)"Status-signals", (char *)"on");
      PREF.activateDevice(sec);
      currentSection = sec;
    }else{
      return;
    }
  }
  currentId = ID;
  QString fourcc, thres, bmin, bmax, irb, stb, sst;
  if(PREF.getKeyVal(sec, (char *)"Threshold", thres)){
    gui.TirThreshold->setValue(thres.toInt());
  }
  if(PREF.getKeyVal(sec, (char *)"Max-blob", bmax)){
    gui.TirMaxBlob->setValue(bmax.toInt());
  }
  if(PREF.getKeyVal(sec, (char *)"Min-blob", bmin)){
    gui.TirMinBlob->setValue(bmin.toInt());
  }
  if(PREF.getKeyVal(sec, (char *)"Ir-led-brightness", irb)){
    gui.TirIrBright->setValue(irb.toInt());
  }
  if(PREF.getKeyVal(sec, (char *)"Status-led-brightness", stb)){
    gui.TirStatusBright->setValue(stb.toInt());
  }
  if(PREF.getKeyVal(sec, (char *)"Status-signals", sst)){
    Qt::CheckState state = (sst.compare("on", Qt::CaseInsensitive) == 0) ? 
                            Qt::Checked : Qt::Unchecked;
    gui.TirSignalizeStatus->setCheckState(state);
  }
  if(haveFirmware()){
    gui.TirFwLabel->setText("Firmware found!");
  }else{
    gui.TirFwLabel->setText("Firmware not found - TrackIr will not work!");
  }
  if(tirType < 5){
    gui.TirIrBright->setDisabled(true);
    gui.TirIrBright->setHidden(true);
    gui.TirStatusBright->setDisabled(true);
    gui.TirStatusBright->setHidden(true);
    gui.StatusBrightLabel->setHidden(true);
    gui.IRBrightLabel->setHidden(true);
  }
}



void TirPrefs::AddAvailableDevices(QComboBox &combo)
{
  QString id;
  deviceType_t dt;
  bool tir_selected = false;
  
  tirType = probeTir();
  if(tirType == 0){
    return;
  }
  
  if(PREF.getActiveDevice(dt,id)){
    if(dt == TIR){
      tir_selected = true;
    }
  }
  
  PrefsLink *pl = new PrefsLink(TIR, (char *)"Tir");
  QVariant v;
  v.setValue(*pl);
  combo.addItem((char *)"TrackIR", v);
  if(tir_selected){
    combo.setCurrentIndex(combo.count() - 1);
  }
}

void TirPrefs::on_TirThreshold_valueChanged(int i)
{
  //std::cout<<"Threshold: "<<i<<std::endl;
  PREF.setKeyVal(currentSection, (char *)"Threshold", i);
}

void TirPrefs::on_TirMinBlob_valueChanged(int i)
{
  //std::cout<<"Min Blob: "<<i<<std::endl;
  PREF.setKeyVal(currentSection, (char *)"Min-blob", i);
}

void TirPrefs::on_TirMaxBlob_valueChanged(int i)
{
  //std::cout<<"Max Blob: "<<i<<std::endl;
  PREF.setKeyVal(currentSection, (char *)"Max-blob", i);
}

void TirPrefs::on_TirStatusBright_valueChanged(int i)
{
  //std::cout<<"Status bright: "<<i<<std::endl;
  PREF.setKeyVal(currentSection, (char *)"Status-led-brightness", i);
}

void TirPrefs::on_TirIrBright_valueChanged(int i)
{
  //std::cout<<"Ir bright: "<<i<<std::endl;
  PREF.setKeyVal(currentSection, (char *)"Ir-led-brightness", i);
}

void TirPrefs::on_TirSignalizeStatus_stateChanged(int state)
{
  QString state_str = (state == Qt::Unchecked) ? "off" : "on";
  //std::cout<<"Signal: "<<state_str.toAscii().data()<<std::endl;
  PREF.setKeyVal(currentSection, (char *)"Status-signals", state_str);
}
