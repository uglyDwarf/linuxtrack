#define NEWS_SERIAL 2

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

#include "ltr_show.h"
#include "ltr_dev_help.h"
#include "ltr_model.h"
#include "ltr_tracking.h"
#include "log_view.h"
#include "help_view.h"
#include "plugin_install.h"
#include "device_setup.h"
#include "profile_selector.h"
#include "guardian.h"
#include "wine_launcher.h"
#include "xplugin.h"

static QMessageBox::StandardButton warnQuestion(const QString &message)
{
 return QMessageBox::warning(NULL, "Linuxtrack",
                                message, QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);
}

static QMessageBox::StandardButton warningMessage(const QString &message)
{
 return QMessageBox::warning(NULL, "Linuxtrack",
                                message, QMessageBox::Ok);
}

static QMessageBox::StandardButton infoMessage(const QString &message)
{
 return QMessageBox::information(NULL, "Linuxtrack",
                                message, QMessageBox::Ok);
}



LinuxtrackGui::LinuxtrackGui(QWidget *parent) : QWidget(parent), ds(NULL),
  xpInstall(NULL), initialized(false), news_serial(-1), guiInit(true)
{
  ui.setupUi(this);
  PREF;
  grd = new Guardian(this);
  me = new ModelEdit(grd, this);
  lv = new LogView();
  pi = new PluginInstall(ui);
  ps = new ProfileSelector(this);
  QObject::connect(&STATE, SIGNAL(stateChanged(ltr_state_type)), 
                   this, SLOT(trackerStateHandler(ltr_state_type)));
  QObject::connect(&zipper, SIGNAL(finished(int, QProcess::ExitStatus)), 
                   this, SLOT(logsPackaged(int, QProcess::ExitStatus)));
  ui.ModelEditSite->addWidget(me);
  ui.ProfileSetupSite->addWidget(ps);
  
  gui_settings = new QSettings("linuxtrack", "ltr_gui");
  showWindow = new LtrGuiForm(ui, *gui_settings);
  helper = new LtrDevHelp();
  gui_settings->beginGroup("MainWindow");
  resize(gui_settings->value("size", QSize(763, 627)).toSize());
  move(gui_settings->value("pos", QPoint(0, 0)).toPoint());
  welcome = gui_settings->value("welcome", true).toBool();
  news_serial = gui_settings->value("news", -1).toInt();
  gui_settings->endGroup();
  gui_settings->beginGroup("TrackingWindow");
  showWindow->resize(gui_settings->value("size", QSize(800, 600)).toSize());
  showWindow->move(gui_settings->value("pos", QPoint(0, 0)).toPoint());
  gui_settings->endGroup();
  gui_settings->beginGroup("HelperWindow");
  helper->resize(gui_settings->value("size", QSize(300, 80)).toSize());
  helper->move(gui_settings->value("pos", QPoint(0, 0)).toPoint());
  gui_settings->endGroup();
  HelpViewer::LoadPrefs(*gui_settings);
  
  ui.LegacyPose->setChecked(ltr_int_use_alter());
  ui.LegacyRotation->setChecked(ltr_int_use_oldrot());
  ui.TransRotDisable->setChecked(!ltr_int_do_tr_align());
  WineLauncher wl;
  if(!wl.check()){
    warningMessage("Wine not working, you'll not be able to install NP firmware and Wine plugin!");
  }
  guiInit = false;
}

void LinuxtrackGui::show()
{
  ds = new DeviceSetup(grd, ui.DeviceSetupSite, this);
  ui.DeviceSetupSite->insertWidget(0, ds);
  showWindow->show();
  QString dbg = QProcessEnvironment::systemEnvironment().value("LINUXTRACK_DBG");
  if(dbg.contains('d')){
    helper->show();
  }
  QWidget::show();
  if(welcome){
    HelpViewer::ChangePage("welcome.htm");
    HelpViewer::ShowWindow();
  }else if(news_serial < NEWS_SERIAL){
    HelpViewer::ChangePage("news.htm");
    HelpViewer::ShowWindow();
  }else{
    HelpViewer::ChangePage("dev_setup.htm");
  }

}

LinuxtrackGui::~LinuxtrackGui()
{
  PrefProxy::ClosePrefs();
}


void LinuxtrackGui::closeEvent(QCloseEvent *event)
{
  HelpViewer::CloseWindow();
  gui_settings->beginGroup("MainWindow");
  gui_settings->setValue("size", size());
  gui_settings->setValue("pos", pos());
  gui_settings->setValue("welcome", false);
  gui_settings->setValue("news", NEWS_SERIAL);
  gui_settings->endGroup();  
  gui_settings->beginGroup("TrackingWindow");
  gui_settings->setValue("size", showWindow->size());
  gui_settings->setValue("pos", showWindow->pos());
  gui_settings->endGroup();  
  gui_settings->beginGroup("HelperWindow");
  gui_settings->setValue("size", helper->size());
  gui_settings->setValue("pos", helper->pos());
  gui_settings->endGroup();
  HelpViewer::StorePrefs(*gui_settings);
  showWindow->StorePrefs(*gui_settings);
  showWindow->allowCloseWindow();
  showWindow->close();
  helper->close();
  lv->close();
  delete pi;
  delete showWindow;
  delete me;
  delete helper;
  delete gui_settings;
  delete ps;
  delete grd;
  delete lv;
  delete ds;
  if(xpInstall != NULL){
  }
  event->accept();
}


void LinuxtrackGui::on_QuitButton_pressed()
{
  TRACKER.stop();
  PREF.SavePrefsOnExit();
  close();
}

void LinuxtrackGui::on_XplanePluginButton_pressed()
{
  if(xpInstall==NULL){
    xpInstall = new XPluginInstall();
  }
  xpInstall->show();
}

void LinuxtrackGui::on_SaveButton_pressed()
{
  PREF.savePrefs();
}

void LinuxtrackGui::on_ViewLogButton_pressed()
{
  lv->show();
}

void LinuxtrackGui::rereadPrefs()
{
  PREF.rereadPrefs();
  if(initialized){
    ds->refresh();
    //track->refresh();
  }
}

void LinuxtrackGui::on_DefaultsButton_pressed()
{
  if(warnQuestion(QString("You are about to load default settings, removing all changes you ever did!\n") + 
                          "Do you really want to do that?") == QMessageBox::Ok){  
    PREF.copyDefaultPrefs();
    rereadPrefs();
    ds->refresh();
  }
}

void LinuxtrackGui::on_DiscardChangesButton_pressed()
{
  if(warnQuestion(QString("You are about to discard modifications you did since last save!\n") + 
                          "Do you really want to do that?") == QMessageBox::Ok){ 
     rereadPrefs();
  }
}

void LinuxtrackGui::on_HelpButton_pressed()
{
  HelpViewer::ShowWindow();
}

void LinuxtrackGui::on_LtrTab_currentChanged(int index)
{
  switch(index){
    case 0:
      HelpViewer::ChangePage("dev_setup.htm");
      break;
    case 1:
      HelpViewer::ChangePage("model_setup.htm");
      break;
    case 2:
      HelpViewer::ChangePage("axes_setup.htm");
      break;
    case 3:
      HelpViewer::ChangePage("misc.htm");
      break;
    default:
      break;
  }
}

void LinuxtrackGui::trackerStateHandler(ltr_state_type current_state)
{
  switch(current_state){
    case STOPPED:
    case ERROR:
      //ui.DeviceSelector->setEnabled(true);
      //ui.CameraOrientation->setEnabled(true);
      //ui.ModelSelector->setEnabled(true);
      //ui.Profiles->setEnabled(true);
      ui.DefaultsButton->setEnabled(true);
      ui.DiscardChangesButton->setEnabled(true);
      //ui.LegacyPose->setEnabled(true);
      //ui.LegacyRotation->setEnabled(true);
      break;
    case INITIALIZING:
    case RUNNING:
    case PAUSED:
      //ui.DeviceSelector->setDisabled(true);
      //ui.CameraOrientation->setDisabled(true);
      //ui.ModelSelector->setDisabled(true);
      //ui.Profiles->setDisabled(true);
      ui.DefaultsButton->setDisabled(true);
      ui.DiscardChangesButton->setDisabled(true);
      //ui.LegacyPose->setDisabled(true);
      //ui.LegacyRotation->setDisabled(true);
      break;
    default:
      break;
  }
}

void LinuxtrackGui::on_LegacyPose_stateChanged(int state)
{
  if(guiInit) return;
  if(state == Qt::Checked){
    ltr_int_set_use_alter(true);
    TRACKER.miscChange(MISC_ALTER, true);
  }else{
    ltr_int_set_use_alter(false);
    TRACKER.miscChange(MISC_ALTER, false);
  }
}

void LinuxtrackGui::on_LegacyRotation_stateChanged(int state)
{
  if(guiInit) return;
  if(state == Qt::Checked){
    ltr_int_set_use_oldrot(true);
    TRACKER.miscChange(MISC_LEGR, true);
  }else{
    ltr_int_set_use_oldrot(false);
    TRACKER.miscChange(MISC_LEGR, false);
  }
}

void LinuxtrackGui::on_TransRotDisable_stateChanged(int state)
{
  if(guiInit) return;
  if(state == Qt::Checked){
    ltr_int_set_tr_align(false);
    TRACKER.miscChange(MISC_ALIGN, false);
  }else{
    ltr_int_set_tr_align(true);
    TRACKER.miscChange(MISC_ALIGN, true);
  }
}


void LinuxtrackGui::on_PackageLogsButton_pressed()
{
  
  QString fname;
  ui.PackageLogsButton->setEnabled(false);
  fname = QFileDialog::getSaveFileName(this, "Save the package as...", QDir::homePath(), "Zip (*.zip)");
  if(fname.isEmpty()){
    return;
  }
  zipper.start(QString("bash -c \"zip %1 /tmp/linuxtrack*.log\"").arg(fname));  
}
  
void LinuxtrackGui::logsPackaged(int exitCode, QProcess::ExitStatus exitStatus)
{
  (void)exitCode;
  if((exitCode == 0) && (exitStatus == QProcess::NormalExit)){
    infoMessage("Package created successfully...");
  }else{
    warningMessage("Couldn't create the package!\n"
    "Please check that you have write access to the destination directory!");
  }
  ui.PackageLogsButton->setEnabled(true);
}



