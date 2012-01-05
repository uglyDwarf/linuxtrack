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
#include "help_view.h"

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


/* Coding:
            bit0 (lsb) - invert camera X values
            bit1       - invert camera Y values
            bit2       - switch X and Y values (applied first!)
            bit4       - invert pitch, roll, X and Z translations (for tracking from behind)
*/
QString LinuxtrackGui::descs[8] = {
    "Normal",                        //0
    "Top to the right",              //6
    "Upside-down",                   //3
    "Top to the left",               //5
    "Normal, from behind",           //8
    "Top to the right, from behind", //14
    "Upside-down, from behind",      //11
    "Top to the left, from behind"   //12
  };

int LinuxtrackGui::orientValues[] = {0, 6, 3, 5, 8, 14, 11, 13};

LinuxtrackGui::LinuxtrackGui(QWidget *parent) : QWidget(parent),
  initialized(false)
{
  ui.setupUi(this);
  PREF; //init prefs
  wcp = new WebcamPrefs(ui);
  wiip = new WiimotePrefs(ui);
  tirp = new TirPrefs(ui);
  me = new ModelEdit(ui);
  track = new LtrTracking(ui);
  sc = new ScpForm();
  lv = new LogView();
//  QObject::connect(this, SIGNAL(customSectionChanged()), sc, SLOT(reinit()));
  
//  QObject::connect(&STATE, SIGNAL(trackerStopped()), this, SLOT(trackerStopped()));
//  QObject::connect(&STATE, SIGNAL(trackerRunning()), this, SLOT(trackerRunning()));
  QObject::connect(&STATE, SIGNAL(stateChanged(ltr_state_type)), this, SLOT(trackerStateHandler(ltr_state_type)));
  
  gui_settings = new QSettings("ltr", "linuxtrack");
  showWindow = new LtrGuiForm(ui, sc, *gui_settings);
  helper = new LtrDevHelp();
  on_RefreshDevices_pressed();
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
  initOrientations();
}

LinuxtrackGui::~LinuxtrackGui()
{
}

void LinuxtrackGui::initOrientations()
{
  int i;
  int orientVal = 0;
  int orientIndex = 0;
  
  QString orient;
  if(PREF.getKeyVal("Global", "Camera-orientation", orient)){
    orientVal=orient.toInt();
  }

  //Initialize Orientations combobox and lookup saved val
  ui.CameraOrientation->clear();
  for(i = 0; i < 8; ++i){
    ui.CameraOrientation->addItem(descs[i]);
    if(orientValues[i] == orientVal){
      orientIndex = i;
    }
  }
  
  ui.CameraOrientation->setCurrentIndex(orientIndex);
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

void LinuxtrackGui::on_CameraOrientation_activated(int index)
{
  if(index < 0){
    return;
  }
  PREF.setKeyVal("Global", "Camera-orientation", orientValues[index]);
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
    on_RefreshDevices_pressed();
    me->refresh();
    track->refresh();
    initOrientations();
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
      ui.DeviceSelector->setEnabled(true);
      ui.CameraOrientation->setEnabled(true);
      ui.ModelSelector->setEnabled(true);
      ui.Profiles->setEnabled(true);
      ui.DefaultsButton->setEnabled(true);
      ui.DiscardChangesButton->setEnabled(true);
      break;
    case INITIALIZING:
    case RUNNING:
    case PAUSED:
      ui.DeviceSelector->setDisabled(true);
      ui.CameraOrientation->setDisabled(true);
      ui.ModelSelector->setDisabled(true);
      ui.Profiles->setDisabled(true);
      ui.DefaultsButton->setDisabled(true);
      ui.DiscardChangesButton->setDisabled(true);
      break;
    default:
      break;
  }
}

