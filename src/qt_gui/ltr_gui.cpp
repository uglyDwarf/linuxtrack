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


LinuxtrackGui::LinuxtrackGui(QWidget *parent) : QWidget(parent),
  initialized(false)
{
  ui.setupUi(this);
  PREF; //init prefs
/*
  wcp = new WebcamPrefs(this);
  wcfp = new WebcamFtPrefs(this);
  wiip = new WiimotePrefs(this);
  tirp = new TirPrefs(this);
*/
  ds = new DeviceSetup(this);
  me = new ModelEdit(this);
  //track = new LtrTracking(ui);
  lv = new LogView();
  pi = new PluginInstall(ui);
  ps = new ProfileSelector(this);
//  QObject::connect(this, SIGNAL(customSectionChanged()), sc, SLOT(reinit()));
  
//  QObject::connect(&STATE, SIGNAL(trackerStopped()), this, SLOT(trackerStopped()));
//  QObject::connect(&STATE, SIGNAL(trackerRunning()), this, SLOT(trackerRunning()));
  QObject::connect(&STATE, SIGNAL(stateChanged(ltr_state_type)), this, SLOT(trackerStateHandler(ltr_state_type)));
/*  ui.DeviceSetupStack->insertWidget(0, wcp);
  ui.DeviceSetupStack->insertWidget(1, wiip);
  ui.DeviceSetupStack->insertWidget(2, tirp);
  ui.DeviceSetupStack->insertWidget(3, wcfp);
*/
  ui.DeviceSetupSite->addWidget(ds);  
  ui.ModelEditSite->addWidget(me);
  ui.ProfileSetupSite->addWidget(ps); 
  
  gui_settings = new QSettings("ltr", "linuxtrack");
  showWindow = new LtrGuiForm(ui, *gui_settings);
  helper = new LtrDevHelp();
  showWindow->show();
  //helper->show();
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
  HelpViewer::LoadPrefs(*gui_settings);
  HelpViewer::ChangePage("dev_setup.htm");
  ui.LegacyPose->setChecked(ltr_int_use_alter());
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
  delete showWindow;
  delete me;
  delete helper;
  delete gui_settings;
  delete ps;
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
  QString fileName = QFileDialog::getOpenFileName(this,
     "Find XPlane executable", "/", "All Files (*)");
  QRegExp pathRexp("^(.*/)[^/]+$");
  if(pathRexp.indexIn(fileName) == -1){
    warningMessage(QString("Strange path... '" + fileName + "'"));
    return;
  }
  QString sourceFile = PrefProxy::getLibPath("xlinuxtrack9");
  QString destPath = pathRexp.cap(1) + "/Resources/plugins";
  if(!QFile::exists(destPath)){
    warningMessage(QString("Wrong file specified!"));
    return;
  }
  
  //Check for the old plugin and remove it if exists
  QString oldPlugin = destPath + "/xlinuxtrack.xpl";
  if(QFile::exists(oldPlugin)){
    if(!QFile::remove(oldPlugin)){
      warningMessage(QString("Can't remove old plugin ('" + oldPlugin + "')!"));
    }
  }
  
  //Create destination path
  destPath += "/xlinuxtrack";
  QDir pluginDir(destPath);
  if(!pluginDir.exists()){
    if(!pluginDir.mkdir(destPath)){
      warningMessage(QString("Can't create new plugin directory ('" + destPath + "')!"));
      return;
    }
  }
  
#ifndef DARWIN
  QString destFile = destPath + "/lin.xpl";
#else
  QString destFile = destPath + "/mac.xpl";
#endif
  QFileInfo fi(destFile);
  if(fi.isFile() || fi.isSymLink()){
    if(!QFile::remove(destFile)){
      warningMessage(QString("Couldn't remove ") + destFile + "!");
      return;
    }
  }else{
    std::cout<<destFile.toAscii().data()<<" is not a file!"<<std::endl;
  }
  if(!QFile::link(sourceFile, destFile)){
    warningMessage(QString("Couldn't link ") + sourceFile + " to " + destFile);
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
      ui.LegacyPose->setEnabled(true);
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
      ui.LegacyPose->setDisabled(true);
      break;
    default:
      break;
  }
}

void LinuxtrackGui::on_LegacyPose_stateChanged(int state)
{
  if(state == Qt::Checked){
    ltr_int_set_use_alter(true);
  }else{
    ltr_int_set_use_alter(false);
  }
}


