#include <plugin_install.h>
#include <ltr_gui_prefs.h>
#include <iostream>
#include <QProcess>
#include <QFileDialog>
#include <QMessageBox>
#include <zlib.h>
#include "extractor.h"
#include "utils.h"

#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

PluginInstall::PluginInstall(const Ui::LinuxtrackMainForm &ui):
  state(DONE), gui(ui), inst(NULL), dlfw(NULL), dlmfc(NULL),
  poem1(PREF.getRsrcDirPath() + QString::fromUtf8("/tir_firmware/poem1.txt")),
  poem2(PREF.getRsrcDirPath() + QString::fromUtf8("/tir_firmware/poem2.txt")),
  gameData(PREF.getRsrcDirPath() + QString::fromUtf8("/tir_firmware/gamedata.txt")),
  mfc42u(PREF.getRsrcDirPath() + QString::fromUtf8("/tir_firmware/mfc42u.dll")),
  tirViews(PREF.getRsrcDirPath() + QString::fromUtf8("/tir_firmware/TIRViews.dll"))
{
#ifndef DARWIN
  if(!QFile::exists(PREF.getDataPath(QString::fromUtf8("linuxtrack-wine.exe")))){
    return;
  }
#endif
  inst = new WineLauncher();
  gui.LinuxtrackWineButton->setEnabled(true);
  Connect();
}

PluginInstall::~PluginInstall()
{
  if(dlfw != NULL){
    delete dlfw;
    dlfw = NULL;
  }
  if(dlmfc != NULL){
    delete dlmfc;
    dlmfc = NULL;
  }
  delete inst;
}

void PluginInstall::Connect()
{
  QObject::connect(gui.LinuxtrackWineButton, SIGNAL(pressed()),
    this, SLOT(installWinePlugin()));
  QObject::connect(gui.TIRFWButton, SIGNAL(pressed()),
    this, SLOT(on_TIRFWButton_pressed()));
  QObject::connect(gui.TIRViewsButton, SIGNAL(pressed()),
    this, SLOT(on_TIRViewsButton_pressed()));
  QObject::connect(inst, SIGNAL(finished(bool)),
    this, SLOT(finished(bool)));
}

void PluginInstall::on_TIRFWButton_pressed()
{
  state = TIR_FW;
  tirFirmwareInstall();
}

void PluginInstall::on_TIRViewsButton_pressed()
{
  state = MFC_ONLY;
  mfc42uInstall();
}

void PluginInstall::installWinePlugin()
{
  if(!isTirFirmwareInstalled()){
    state = TIR_FW;
    tirFirmwareInstall();
  }else if(!isMfc42uInstalled()){
    state = MFC;
    mfc42uInstall();
  }else{
    installLinuxtrackWine();
    state = LTR_W;
  }
}



bool PluginInstall::isTirFirmwareInstalled()
{
  return QFile::exists(poem1) && QFile::exists(poem2) && QFile::exists(gameData) && QFile::exists(tirViews);
}

bool PluginInstall::isMfc42uInstalled()
{
  return QFile::exists(mfc42u);
}


void PluginInstall::installLinuxtrackWine()
{
  if(dlfw != NULL){
    dlfw->hide();
  }
  if(dlmfc != NULL){
    dlmfc->hide();
  }
#ifndef DARWIN
  QString prefix = QFileDialog::getExistingDirectory(NULL, QString::fromUtf8("Select Wine Prefix..."),
                     QDir::homePath(), QFileDialog::ShowDirsOnly);
  QString installerPath = PREF.getDataPath(QString::fromUtf8("linuxtrack-wine.exe"));

  inst->setEnv(QString::fromUtf8("WINEPREFIX"), prefix);
  inst->run(installerPath);
#else
  if(isTirFirmwareInstalled() && isMfc42uInstalled()){
    QMessageBox::information(NULL, QString::fromUtf8("Firmware extraction successfull"),
      QString::fromUtf8("Firmware extraction finished successfully!"
      "\nNow you can install linuxtrack-wine.exe to the Wine bottle/prefix of your choice."
      )
    );
  }
#endif
  gui.LinuxtrackWineButton->setEnabled(true);
}

void PluginInstall::tirFirmwareInstall()
{
  if(dlfw == NULL){
    dlfw = new TirFwExtractor();
    QObject::connect(dlfw, SIGNAL(finished(bool)),
      this, SLOT(finished(bool)));
  }
  dlfw->show();
}

void PluginInstall::mfc42uInstall()
{
  if(!isTirFirmwareInstalled()){
    QMessageBox::warning(NULL, QString::fromUtf8("Mfc42u install"),
                         QString::fromUtf8("Install TrackIR firmware first!"));
    state = TIR_FW;
    tirFirmwareInstall();
    return;
  }
  if(dlmfc == NULL){
    dlmfc = new Mfc42uExtractor();
    QObject::connect(dlmfc, SIGNAL(finished(bool)),
      this, SLOT(finished(bool)));
  }
  dlmfc->show();
}

void PluginInstall::finished(bool ok)
{
  (void) ok;
  if(dlfw != NULL){
    dlfw->hide();
  }
  if(dlmfc != NULL){
    dlmfc->hide();
  }
  switch(state){
    case TIR_FW:
      state = MFC;
      mfc42uInstall();
      break;
    case MFC:
      state = LTR_W;
      installLinuxtrackWine();
      break;
    case LTR_W:
    case TIR_FW_ONLY:
    case MFC_ONLY:
    default:
      state = DONE;
      enableButtons(true);
      break;
  }
}

void PluginInstall::enableButtons(bool ena)
{
  gui.LinuxtrackWineButton->setEnabled(ena);
  gui.TIRFWButton->setEnabled(ena);
  gui.TIRViewsButton->setEnabled(ena);
}
