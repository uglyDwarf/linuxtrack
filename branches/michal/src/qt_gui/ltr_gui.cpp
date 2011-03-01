#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
#include <QSettings>
#include <iostream>
#include "ltr_gui.h"
#include "ltr_gui_prefs.h"
#include "prefs_link.h"
#include "pathconfig.h"
#include "ltr_state.h"

#include "tir_prefs.h"
#include "ltr_show.h"
#include "ltr_dev_help.h"
#include "ltr_model.h"
#include "ltr_tracking.h"
#include "log_view.h"
#include "scp_form.h"
#include "webcam_prefs.h"
#include "wiimote_prefs.h"

LinuxtrackGui::LinuxtrackGui(QWidget *parent) : QWidget(parent),
  initialized(false)
{
  ui.setupUi(this);
  QString target = PrefProxy::getRsrcDirPath() + "/linuxtrack.conf";
  if(!QFileInfo(target).isReadable()){
    on_DefaultsButton_pressed();
  }
  wcp = new WebcamPrefs(ui);
  wiip = new WiimotePrefs(ui);
  tirp = new TirPrefs(ui);
  me = new ModelEdit(ui);
  track = new LtrTracking(ui);
  sc = new ScpForm();
  lv = new LogView();
//  QObject::connect(this, SIGNAL(customSectionChanged()), sc, SLOT(reinit()));
  
  QObject::connect(&STATE, SIGNAL(trackerStopped()), this, SLOT(trackerStopped()));
  QObject::connect(&STATE, SIGNAL(trackerRunning()), this, SLOT(trackerRunning()));
  
  showWindow = new LtrGuiForm(ui, sc);
  helper = new LtrDevHelp();
  on_RefreshDevices_pressed();
  showWindow->show();
  helper->show();
  gui_settings = new QSettings("ltr", "linuxtrack");
  gui_settings->beginGroup("MainWindow");
  resize(gui_settings->value("size", QSize(763, 627)).toSize());
  move(gui_settings->value("pos", QPoint(100, 100)).toPoint());
  gui_settings->endGroup();
  gui_settings->beginGroup("TrackingWindow");
  showWindow->resize(gui_settings->value("size", QSize(800, 600)).toSize());
  showWindow->move(gui_settings->value("pos", QPoint(10, 10)).toPoint());
  gui_settings->endGroup();
  gui_settings->beginGroup("HelperWindow");
  helper->resize(gui_settings->value("size", QSize(300, 80)).toSize());
  helper->move(gui_settings->value("pos", QPoint(0, 0)).toPoint());
  gui_settings->endGroup();
}

LinuxtrackGui::~LinuxtrackGui()
{
}

void LinuxtrackGui::closeEvent(QCloseEvent *event)
{
  gui_settings->beginGroup("MainWindow");
  gui_settings->setValue("size", size());
  gui_settings->setValue("pos", pos());
  gui_settings->endGroup();  
  gui_settings->beginGroup("TrackingWindow");
  gui_settings->setValue("size", showWindow->size());
  gui_settings->setValue("pos", showWindow->pos());
  gui_settings->endGroup();  
  gui_settings->beginGroup("HelperWindow");
  gui_settings->setValue("size", helper->size());
  gui_settings->setValue("pos", helper->pos());
  gui_settings->endGroup();  
  showWindow->allowCloseWindow();
  showWindow->close();
  helper->close();
  sc->close();
  lv->close();
  delete wcp;
  delete wiip;
  delete tirp;
  delete showWindow;
  delete me;
  delete sc;
  delete helper;
  PrefProxy::ClosePrefs();
  event->accept();
}

void LinuxtrackGui::on_DeviceSelector_activated(int index)
{
  if(index < 0){
    return;
  }
  QVariant v = ui.DeviceSelector->itemData(index);
  PrefsLink pl = v.value<PrefsLink>();
  if(pl.deviceType == WEBCAM){
#ifndef DARWIN
    ui.DeviceSetupStack->setCurrentIndex(0);
#else
    ui.DeviceSetupStack->setCurrentIndex(3);
#endif
    wcp->Activate(pl.ID, !initialized);
  }else 
  if(pl.deviceType == WIIMOTE){
    ui.DeviceSetupStack->setCurrentIndex(1);
    wiip->Activate(pl.ID, !initialized);
  }else 
  if(pl.deviceType == TIR){
    ui.DeviceSetupStack->setCurrentIndex(2);
    tirp->Activate(pl.ID, !initialized);
  }
}

void LinuxtrackGui::on_RefreshDevices_pressed()
{
  ui.DeviceSelector->clear();
  bool res = false; 
  res |= WebcamPrefs::AddAvailableDevices(*(ui.DeviceSelector));
  res |= WiimotePrefs::AddAvailableDevices(*(ui.DeviceSelector));
  res |= TirPrefs::AddAvailableDevices(*(ui.DeviceSelector));
  if(!res){
    initialized = true;
  }
  on_DeviceSelector_activated(ui.DeviceSelector->currentIndex());
  initialized = true;
}

void LinuxtrackGui::on_QuitButton_pressed()
{
  close();
}

void LinuxtrackGui::on_EditSCButton_pressed()
{
  sc->show();
}

static int warnMessage(const QString &message){
 return QMessageBox::warning(NULL, "Linuxtrack",
                                message, QMessageBox::Ok, QMessageBox::Ok);
}

void LinuxtrackGui::on_XplanePluginButton_pressed()
{
  QString fileName = QFileDialog::getOpenFileName(this,
     "Find XPlane executable", "/", "All Files (*)");
  QRegExp pathRexp("^(.*/)[^/]+$");
  if(pathRexp.indexIn(fileName) == -1){
    warnMessage(QString("Strange path... '" + fileName + "'"));
    return;
  }
  QString sourceFile = PrefProxy::getLibPath("linuxtrack/xlinuxtrack9");
  QString destPath = pathRexp.cap(1) + "/Resources/plugins";
  if(!QFile::exists(destPath)){
    warnMessage(QString("Wrong file specified!"));
    return;
  }
  QString destFile = destPath + "/xlinuxtrack.xpl";
  QFileInfo fi(destFile);
  if(fi.isFile() || fi.isSymLink()){
    if(!QFile::remove(destFile)){
      warnMessage(QString("Couldn't remove ") + destFile + "!");
      return;
    }
  }else{
    std::cout<<destFile.toAscii().data()<<" is not a file!"<<std::endl;
  }
  if(!QFile::link(sourceFile, destFile)){
    warnMessage(QString("Couldn't link ") + sourceFile + " to " + destFile);
  }
}

void LinuxtrackGui::on_SaveButton_pressed()
{
  PREF.savePrefs();
}

void LinuxtrackGui::on_ViewLogButton_pressed()
{
  lv->show();
}

void LinuxtrackGui::on_DefaultsButton_pressed()
{
  QString targetDir = PrefProxy::getRsrcDirPath();
  if(targetDir.endsWith("/")){
    targetDir.chop(1);
  }
  QString target = targetDir + "/linuxtrack.conf";
  QString source = PrefProxy::getDataPath("linuxtrack.conf");
  if(QFileInfo(targetDir).isFile()){//old setup
    QFile::rename(targetDir, targetDir + ".backup");
    QDir::home().mkpath(targetDir);
    QFile::rename(targetDir + ".backup", target);
  }else if(QFileInfo(targetDir).isDir()){
    QFileInfo conf(target);
    if(!conf.isFile()){
      QFile::copy(source, target);
    }
  }else{
    QDir::home().mkdir(".linuxtrack");
    QFile::copy(source, target);
  }
//  std::cout<<"Going to copy "<<source.toAscii().data();
//  std::cout<<" to "<<target.toAscii().data()<<std::endl;
  on_DiscardChangesButton_pressed();
}

void LinuxtrackGui::on_DiscardChangesButton_pressed()
{
  PREF.rereadPrefs();
  if(initialized){
    on_RefreshDevices_pressed();
    me->refresh();
    track->refresh();
  }
}

  
void LinuxtrackGui::trackerStopped()
{
  ui.DeviceSelector->setEnabled(true);
  ui.ModelSelector->setEnabled(true);
  ui.Profiles->setEnabled(true);
  ui.DefaultsButton->setEnabled(true);
}

void LinuxtrackGui::trackerRunning()
{
  ui.DeviceSelector->setDisabled(true);
  ui.ModelSelector->setDisabled(true);
  ui.Profiles->setDisabled(true);
  ui.DefaultsButton->setDisabled(true);
}

