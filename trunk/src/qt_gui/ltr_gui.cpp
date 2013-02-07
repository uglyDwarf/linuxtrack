#define NEWS_SERIAL 1

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
  initialized(false), news_serial(-1)
{
  ui.setupUi(this);
  PREF;
  grd = new Guardian(this);
  ds = new DeviceSetup(grd, this);
  me = new ModelEdit(grd, this);
  lv = new LogView();
  pi = new PluginInstall(ui);
  ps = new ProfileSelector(this);
  QObject::connect(&STATE, SIGNAL(stateChanged(ltr_state_type)), this, SLOT(trackerStateHandler(ltr_state_type)));
  ui.DeviceSetupSite->addWidget(ds);  
  ui.ModelEditSite->addWidget(me);
  ui.ProfileSetupSite->addWidget(ps);
  
  gui_settings = new QSettings("linuxtrack", "ltr_gui");
  showWindow = new LtrGuiForm(ui, *gui_settings);
  helper = new LtrDevHelp();
  showWindow->show();
  helper->show();
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
}

void LinuxtrackGui::show()
{
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
  delete showWindow;
  delete me;
  delete helper;
  delete gui_settings;
  delete ps;
  delete grd;
  event->accept();
}


void LinuxtrackGui::on_QuitButton_pressed()
{
  TRACKER.stop();
  PREF.SavePrefsOnExit();
  close();
}

static void warn(const QString baseMsg, const QString explanation)
{
  warningMessage(QString("%1\nSystem says: %2").arg(baseMsg).arg(explanation));
}

bool removePlugin(const QString targetName)
{
  QFile target(targetName);
  if(target.exists()){
    if(!target.remove()){
      warn(QString("Can't remove old plugin '%1'!").arg(targetName), target.errorString());
      return false;
    }
  }
  return true;
}


static bool installPlugin(const QString sourceFile, const QString destFile)
{
  //Create destination path
  QFile src(sourceFile);
  QFile dest(destFile);
  if(!src.exists()){
    warningMessage(QString("Source file '%s' doesn't exist!"));
    return false;
  }
  QFileInfo destInfo(destFile);
  QDir destDir = destInfo.dir();
  //make sure the destination path exists
  if(!destDir.exists()){
    if(!destDir.mkpath(destDir.path())){
      warningMessage(QString("Can't create output directory '%1'!").arg(destDir.path()));
      return false;
    }
  }
  //check if the file exists already
  if(dest.exists()){
    if(!removePlugin(destFile)){
      return false;
    }
  }
  //copy the new file
  if(!src.copy(destFile)){
    warn(QString("Can't copy file '%1' to '%2'!").arg(destFile).arg(destDir.path()), src.errorString());
    return false;
  }
  return true;
}

void LinuxtrackGui::on_XplanePluginButton_pressed()
{
  QString fileName = QFileDialog::getOpenFileName(this,
     "Find XPlane executable", "/", "All Files (*)");
  QRegExp pathRexp("^(.*/)[^/]+$");
  if(pathRexp.indexIn(fileName) == -1){
    warningMessage(QString("This doesn't seem to be the right path... '" + fileName + "'"));
    return;
  }
  QString sourceFile32 = PrefProxy::getLibPath("xlinuxtrack9_32");
  QString sourceFile = PrefProxy::getLibPath("xlinuxtrack9");
  QString destPath = pathRexp.cap(1) + "/Resources/plugins";
  if(!QFile::exists(destPath)){
    warningMessage(QString("Wrong file specified!"));
    return;
  }
  
  //Check for the old plugin and remove it if exists
  QString oldPlugin = destPath + "/xlinuxtrack.xpl";
  QFileInfo old(oldPlugin);
  if(old.exists()){
    if(!removePlugin(oldPlugin)){
      return;
    }
  }
  destPath += "/xlinuxtrack/";
#ifndef DARWIN
  if(installPlugin(sourceFile32, destPath + "/lin.xpl") &&
       installPlugin(sourceFile, destPath + "/64/lin.xpl")){
#else
  if(installPlugin(sourceFile, destPath + "/mac.xpl")){
#endif
    QMessageBox::information(NULL, "Linuxtrack", "XPlane plugin installed successfuly!");
  }else{
    warningMessage(QString("XPlane plugin installation failed!"));
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


