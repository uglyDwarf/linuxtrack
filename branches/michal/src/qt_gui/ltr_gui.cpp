#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
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

#ifndef DARWIN
  #include "webcam_prefs.h"
  #include "wiimote_prefs.h"
#endif

LinuxtrackGui::LinuxtrackGui(QWidget *parent) : QWidget(parent),
  initialized(false)
{
  ui.setupUi(this);
  QString target = QDir::homePath() + "/.linuxtrack";
  if(!QFile::exists(target)){
    on_DefaultsButton_pressed();
  }
#ifndef DARWIN
  wcp = new WebcamPrefs(ui);
  wiip = new WiimotePrefs(ui);
#endif
  tirp = new TirPrefs(ui);
  me = new ModelEdit(ui);
  track = new LtrTracking(ui);
  sc = new ScpForm();
  lv = new LogView();
//  QObject::connect(this, SIGNAL(customSectionChanged()), sc, SLOT(reinit()));
  
  QObject::connect(&STATE, SIGNAL(trackerStopped()), this, SLOT(trackerStopped()));
  QObject::connect(&STATE, SIGNAL(trackerRunning()), this, SLOT(trackerRunning()));
  
  showWindow = new LtrGuiForm(sc);
  helper = new LtrDevHelp();
  initialized = true;
  on_RefreshDevices_pressed();
  showWindow->show();
  helper->show();
}

LinuxtrackGui::~LinuxtrackGui()
{
  delete showWindow;
#ifndef DARWIN
  delete wcp;
  delete wiip;
#endif
  delete tirp;
  delete me;
  delete sc;
  delete helper;
}

void LinuxtrackGui::closeEvent(QCloseEvent *event)
{
  showWindow->allowCloseWindow();
  showWindow->close();
  helper->close();
  sc->close();
  lv->close();
  event->accept();
}

void LinuxtrackGui::on_DeviceSelector_activated(int index)
{
  if(index < 0){
    return;
  }
  QVariant v = ui.DeviceSelector->itemData(index);
  PrefsLink pl = v.value<PrefsLink>();
#ifndef DARWIN
  if(pl.deviceType == WEBCAM){
    ui.DeviceSetupStack->setCurrentIndex(0);
    wcp->Activate(pl.ID);
  }else if(pl.deviceType == WIIMOTE){
    ui.DeviceSetupStack->setCurrentIndex(1);
    wiip->Activate(pl.ID);
  }else 
#endif
  if(pl.deviceType == TIR){
    ui.DeviceSetupStack->setCurrentIndex(2);
    tirp->Activate(pl.ID);
  }
}

void LinuxtrackGui::on_RefreshDevices_pressed()
{
  ui.DeviceSelector->clear();
#ifndef DARWIN
  WebcamPrefs::AddAvailableDevices(*(ui.DeviceSelector));
  WiimotePrefs::AddAvailableDevices(*(ui.DeviceSelector));
#endif
  TirPrefs::AddAvailableDevices(*(ui.DeviceSelector));
  on_DeviceSelector_activated(ui.DeviceSelector->currentIndex());
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
  QString sourceFile = PrefProxy::getLibPath("xlinuxtrack");
  QString destPath = pathRexp.cap(1) + "/Resources/plugins";
  if(!QFile::exists(destPath)){
    warnMessage(QString("Wrong file specified!"));
    return;
  }
  QString destFile = destPath + "/xlinuxtrack.xpl";
  if(QFile::exists(destFile)){
    if(!QFile::remove(destFile)){
      warnMessage(QString("Couldn't remove ") + destFile + "!");
      return;
    }
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
  QString target = QDir::homePath() + "/.linuxtrack";
  QString source = PrefProxy::getDataPath(".linuxtrack");
  std::cout<<"Going to copy "<<source.toAscii().data();
  std::cout<<" to "<<target.toAscii().data()<<std::endl;
  QFile::rename(target, target + ".backup");
  QFile::copy(source, target);
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

