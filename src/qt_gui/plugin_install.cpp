#include <plugin_install.h>
#include <ltr_gui_prefs.h>
#include <iostream>
#include <QProcess>
#include <QFileDialog>
#include <QMessageBox>
#include <zlib.h>
#include "dlfirmware.h"
#include "utils.h"

const QString PluginInstall::keySrc(PREF.getRsrcDirPath() + "/tir_firmware/tir4.fw.gz");
const QString PluginInstall::keyFile(PREF.getRsrcDirPath() + "/tir_firmware/sig.key");
const QString PluginInstall::sigFile(PREF.getRsrcDirPath() + "/tir_firmware/sig.bin");

PluginInstall::PluginInstall(const Ui::LinuxtrackMainForm &ui):
  gui(ui), inst(NULL)
{
  if(QFile::exists(PREF.getDataPath("linuxtrack-wine.exe"))){
    inst = new QProcess(this);
    gui.pushButton_2->setEnabled(true);
    Connect();
  }
}

PluginInstall::~PluginInstall()
{
  if(dlfw != NULL){
    delete dlfw;
    dlfw = NULL;
  }
  delete inst;
}

void PluginInstall::Connect()
{
  QObject::connect(gui.pushButton_2, SIGNAL(pressed()),
    this, SLOT(installWinePlugin()));
  QObject::connect(inst, SIGNAL(finished(int, QProcess::ExitStatus)),
    this, SLOT(instFinished(int, QProcess::ExitStatus)));
}

void PluginInstall::instFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  (void) exitCode;
  (void) exitStatus;
  if(!QFile::exists(keySrc)){
    printf("Key source not existing!\n");
    tirFirmwareInstall();
  }else{
    printf("We have key source, continue with extraction...\n");
    tirFirmwareInstalled(false);
  }
  gui.pushButton_2->setEnabled(true);
}

void PluginInstall::tirFirmwareInstall()
{
  if(dlfw == NULL){
    dlfw = new dlfwGui();
    QObject::connect(dlfw, SIGNAL(finished(bool)),
      this, SLOT(tirFirmwareInstalled(bool)));
  }
  dlfw->show();
}

static bool saveKeyFile(const char *fname, const char *target)
{
  bool res = false;
  uint8_t buf[400];
  gzFile gf;
  if((gf = gzopen(fname, "rb")) == NULL){
    ltr_int_log_message("Couldn't open keyfile '%s'\n", fname);
    return false;
  }
  size_t read = gzread(gf, buf, sizeof(buf));
  res = (read == sizeof(buf));
  gzclose(gf);
  if(res == false){
    return false;
  }
  res = false;
  for(unsigned int i = 0; i < sizeof(buf); ++i) buf[i] ^= 0x5A;
  FILE *f = fopen(target, "wb");
  if(f != NULL){
    res = (fwrite(buf, 1, sizeof(buf), f) == sizeof(buf));
    fclose(f);
  }else{
    ltr_int_log_message("Can't open target file '%s'!\n", target);
  }
  return res;
}

bool PluginInstall::isTirFirmwareInstalled()
{
  return QFile::exists(keyFile) || QFile::exists(sigFile);
}

void PluginInstall::tirFirmwareInstalled(bool ok)
{
  (void) ok;
  bool res = false;
  if(QFile::exists(keySrc) && (!QFile::exists(sigFile))){
    res = saveKeyFile(qPrintable(keySrc), qPrintable(keyFile));
  }
  if(!(res || QFile::exists(sigFile))){
    QMessageBox::warning(NULL, "TrackIR firmware install problem", 
"TrackIR firmware package was not installed, without it\n\
the linuxtrack-wine bridge will not be fully functional!", QMessageBox::Ok);
  }
}

void PluginInstall::installWinePlugin()
{
  gui.pushButton_2->setEnabled(false);
  QString prefix = QFileDialog::getExistingDirectory(NULL, QString("Select Wine Prefix..."), 
                     QDir::homePath()+"/.wine", QFileDialog::ShowDirsOnly);
  QString installerPath = PREF.getDataPath("linuxtrack-wine.exe");
  
  QString program = "/bin/bash";
  QString arg = QString("WINEPREFIX=") + prefix + " wine " + installerPath;
  QStringList args;
  args << "-c" << arg;
  inst->start(program, args);
}


