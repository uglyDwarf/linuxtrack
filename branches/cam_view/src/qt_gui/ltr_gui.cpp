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
#include "wine_warn.h"

static QMessageBox::StandardButton warnQuestion(const QString &message)
{
 return QMessageBox::warning(NULL, QString::fromUtf8("Linuxtrack"),
                                message, QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);
}

static QMessageBox::StandardButton warningMessage(const QString &message)
{
 return QMessageBox::warning(NULL, QString::fromUtf8("Linuxtrack"),
                                message, QMessageBox::Ok);
}

static QMessageBox::StandardButton infoMessage(const QString &message)
{
 return QMessageBox::information(NULL, QString::fromUtf8("Linuxtrack"),
                                message, QMessageBox::Ok);
}



LinuxtrackGui::LinuxtrackGui(QWidget *parent) : QWidget(parent), ds(NULL),
  xpInstall(NULL), initialized(false), news_serial(-1), guiInit(true), showWineWarning(true)
{
  ui.setupUi(this);
  PREF;
  setWindowTitle(QString::fromUtf8("Linuxtrack GUI v") + QString::fromUtf8(PACKAGE_VERSION));
  grd = new Guardian(this);
  me = new ModelEdit(grd, this);
  lv = new LogView();
  pi = new PluginInstall(ui, this);
  ps = new ProfileSelector(this);
  QObject::connect(&STATE, SIGNAL(stateChanged(linuxtrack_state_type)),
                   this, SLOT(trackerStateHandler(linuxtrack_state_type)));
  QObject::connect(&zipper, SIGNAL(finished(int, QProcess::ExitStatus)),
                   this, SLOT(logsPackaged(int, QProcess::ExitStatus)));
  ui.ModelEditSite->addWidget(me);
  ui.ProfileSetupSite->addWidget(ps);

  gui_settings = new QSettings(QString::fromUtf8("linuxtrack"), QString::fromUtf8("ltr_gui"));
  showWindow = new LtrGuiForm(ui, *gui_settings);
  helper = new LtrDevHelp();
  gui_settings->beginGroup(QString::fromUtf8("MainWindow"));
  resize(gui_settings->value(QString::fromUtf8("size"), QSize(763, 627)).toSize());
  move(gui_settings->value(QString::fromUtf8("pos"), QPoint(0, 0)).toPoint());
  welcome = gui_settings->value(QString::fromUtf8("welcome"), true).toBool();
  news_serial = gui_settings->value(QString::fromUtf8("news"), -1).toInt();
  showWineWarning = gui_settings->value(QString::fromUtf8("wine_warning"), true).toBool();
  gui_settings->endGroup();
  gui_settings->beginGroup(QString::fromUtf8("TrackingWindow"));
  showWindow->resize(gui_settings->value(QString::fromUtf8("size"), QSize(800, 600)).toSize());
  showWindow->move(gui_settings->value(QString::fromUtf8("pos"), QPoint(0, 0)).toPoint());
  gui_settings->endGroup();
  gui_settings->beginGroup(QString::fromUtf8("HelperWindow"));
  helper->resize(gui_settings->value(QString::fromUtf8("size"), QSize(300, 80)).toSize());
  helper->move(gui_settings->value(QString::fromUtf8("pos"), QPoint(0, 0)).toPoint());
  gui_settings->endGroup();
  HelpViewer::LoadPrefs(*gui_settings);

  ui.LegacyPose->setChecked(ltr_int_use_alter());
  ui.LegacyRotation->setChecked(ltr_int_use_oldrot());
  ui.TransRotDisable->setChecked(!ltr_int_do_tr_align());
  ui.FocalLength->setValue(ltr_int_get_focal_length());
  WineLauncher wl;
  if(!wl.wineAvailable() && showWineWarning){
    WineWarn w(this);
    if(w.exec() == QDialog::Accepted){
      showWineWarning = false;
    }
  }
  guiInit = false;
}

void LinuxtrackGui::show()
{
  ds = new DeviceSetup(grd, ui.DeviceSetupSite, this);
  ui.DeviceSetupSite->insertWidget(0, ds);
  showWindow->show();
  QString dbg = QProcessEnvironment::systemEnvironment().value(QString::fromUtf8("LINUXTRACK_DBG"));
  if(dbg.contains(QChar::fromLatin1('d'))){
    helper->show();
  }
  QWidget::show();
  if(welcome){
    HelpViewer::ChangePage(QString::fromUtf8("welcome.htm"));
    HelpViewer::ShowWindow();
  }else if(news_serial < NEWS_SERIAL){
    HelpViewer::ChangePage(QString::fromUtf8("news.htm"));
    HelpViewer::ShowWindow();
  }else{
    HelpViewer::ChangePage(QString::fromUtf8("dev_setup.htm"));
  }

}

LinuxtrackGui::~LinuxtrackGui()
{
  PrefProxy::ClosePrefs();
  delete pi;
  pi = NULL;
  delete showWindow;
  showWindow = NULL;
  delete me;
  me = NULL;
  delete helper;
  helper = NULL;
  delete gui_settings;
  gui_settings = NULL;
  delete ps;
  ps = NULL;
  delete grd;
  grd = NULL;
  delete lv;
  lv = NULL;
  delete ds;
  ds = NULL;
  if(xpInstall != NULL){
  }
}


void LinuxtrackGui::closeEvent(QCloseEvent *event)
{
  static bool invokedAlready = false;
  if(invokedAlready){
    event->accept();
    return;
  }
  invokedAlready = true;
  TRACKER.stop();
  PREF.SavePrefsOnExit();
  HelpViewer::CloseWindow();
  gui_settings->beginGroup(QString::fromUtf8("MainWindow"));
  gui_settings->setValue(QString::fromUtf8("size"), size());
  gui_settings->setValue(QString::fromUtf8("pos"), pos());
  gui_settings->setValue(QString::fromUtf8("welcome"), false);
  gui_settings->setValue(QString::fromUtf8("news"), NEWS_SERIAL);
  gui_settings->setValue(QString::fromUtf8("wine_warning"), showWineWarning);
  gui_settings->endGroup();
  gui_settings->beginGroup(QString::fromUtf8("TrackingWindow"));
  gui_settings->setValue(QString::fromUtf8("size"), showWindow->size());
  gui_settings->setValue(QString::fromUtf8("pos"), showWindow->pos());
  gui_settings->endGroup();
  gui_settings->beginGroup(QString::fromUtf8("HelperWindow"));
  gui_settings->setValue(QString::fromUtf8("size"), helper->size());
  gui_settings->setValue(QString::fromUtf8("pos"), helper->pos());
  gui_settings->endGroup();
  HelpViewer::StorePrefs(*gui_settings);
  showWindow->StorePrefs(*gui_settings);
  showWindow->allowCloseWindow();
  showWindow->close();
  helper->close();
  lv->close();
  ps->close();
  pi->close();
  event->accept();
}


void LinuxtrackGui::on_QuitButton_pressed()
{
  close();
}

void LinuxtrackGui::on_XplanePluginButton_pressed()
{
  if(xpInstall==NULL){
    xpInstall = new XPluginInstall();
  }
  xpInstall->exec();
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
  if(warnQuestion(QString::fromUtf8("You are about to load default settings, removing all changes you ever did!\n") +
                          QString::fromUtf8("Do you really want to do that?")) == QMessageBox::Ok){
    PREF.copyDefaultPrefs();
    rereadPrefs();
    ds->refresh();
  }
}

void LinuxtrackGui::on_DiscardChangesButton_pressed()
{
  if(warnQuestion(QString::fromUtf8("You are about to discard modifications you did since last save!\n") +
                          QString::fromUtf8("Do you really want to do that?")) == QMessageBox::Ok){
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
      HelpViewer::ChangePage(QString::fromUtf8("dev_setup.htm"));
      break;
    case 1:
      HelpViewer::ChangePage(QString::fromUtf8("model_setup.htm"));
      break;
    case 2:
      HelpViewer::ChangePage(QString::fromUtf8("axes_setup.htm"));
      break;
    case 3:
      HelpViewer::ChangePage(QString::fromUtf8("misc.htm"));
      break;
    default:
      break;
  }
}

void LinuxtrackGui::trackerStateHandler(linuxtrack_state_type current_state)
{
  switch(current_state){
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
      //ui.DeviceSelector->setEnabled(true);
      //ui.CameraOrientation->setEnabled(true);
      //ui.ModelSelector->setEnabled(true);
      //ui.Profiles->setEnabled(true);
      ui.DefaultsButton->setEnabled(true);
      ui.DiscardChangesButton->setEnabled(true);
      //ui.LegacyPose->setEnabled(true);
      //ui.LegacyRotation->setEnabled(true);
      break;
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

void LinuxtrackGui::on_FocalLength_valueChanged(double val)
{
  if(guiInit) return;
  ltr_int_set_focal_length(val);
  TRACKER.miscChange(MISC_FOCAL_LENGTH, (float)val);
}

void LinuxtrackGui::on_PackageLogsButton_pressed()
{
  QString fname;
  ui.PackageLogsButton->setEnabled(false);
  fname = QFileDialog::getSaveFileName(this, QString::fromUtf8("Save the package as..."),
                                       QDir::homePath(), QString::fromUtf8("Zip (*.zip)"));
  if(fname.isEmpty()){
    return;
  }
  zipper.start(QString::fromUtf8("bash -c \"zip %1 /tmp/linuxtrack*.log*\"").arg(fname));
}

void LinuxtrackGui::logsPackaged(int exitCode, QProcess::ExitStatus exitStatus)
{
  (void)exitCode;
  if((exitCode == 0) && (exitStatus == QProcess::NormalExit)){
    infoMessage(QString::fromUtf8("Package created successfully..."));
  }else{
    warningMessage(QString::fromUtf8("Couldn't create the package!\n"
    "Please check that you have write access to the destination directory!"));
  }
  ui.PackageLogsButton->setEnabled(true);
}



