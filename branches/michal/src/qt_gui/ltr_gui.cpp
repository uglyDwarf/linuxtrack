#include <QFileDialog>
#include <QMessageBox>
#include <iostream>
#include "ltr_gui.h"
#include "ltr_gui_prefs.h"
#include "prefs_link.h"
#include "pathconfig.h"

LinuxtrackGui::LinuxtrackGui(QWidget *parent) : QWidget(parent)
{
  ui.setupUi(this);
  wcp = new WebcamPrefs(ui);
  wiip = new WiimotePrefs(ui);
  tirp = new TirPrefs(ui);
  me = new ModelEdit(ui);
  sc = new ScpForm();
  sc->setSlaves(ui.PitchUpSpin, ui.PitchDownSpin,
                ui.TiltLeftSpin, ui.TiltRightSpin,
                ui.YawLeftSpin, ui.YawRightSpin,
                ui.MoveLeftSpin, ui.MoveRightSpin,
                ui.MoveUpSpin, ui.MoveDownSpin,
                ui.MoveBackSpin, ui.MoveForthSpin
                );
  
  helper = new LtrDevHelp(sc);
  on_RefreshDevices_pressed();
  showWindow.show();
  helper->show();
  
  
}

LinuxtrackGui::~LinuxtrackGui()
{
  delete wcp;
  delete wiip;
  delete tirp;
  delete me;
  delete sc;
  delete helper;
}

void LinuxtrackGui::closeEvent(QCloseEvent *event)
{
  showWindow.close();
  helper->close();
  sc->close();
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
    ui.DeviceSetupStack->setCurrentIndex(0);
    wcp->Activate(pl.ID);
  }else if(pl.deviceType == WIIMOTE){
    ui.DeviceSetupStack->setCurrentIndex(1);
    wiip->Activate(pl.ID);
  }else if(pl.deviceType == TIR){
    ui.DeviceSetupStack->setCurrentIndex(2);
    tirp->Activate(pl.ID);
  }
}

void LinuxtrackGui::on_RefreshDevices_pressed()
{
  ui.DeviceSelector->clear();
  WebcamPrefs::AddAvailableDevices(*(ui.DeviceSelector));
  WiimotePrefs::AddAvailableDevices(*(ui.DeviceSelector));
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
  QString sourceFile = QString(PREFIX) + "/lib/xlinuxtrack.so";
  QString destPath = pathRexp.cap(1) + "/Resources/plugins";
  if(!QFile::exists(destPath)){
    warnMessage(QString("Wrong file specified!"));
    return;
  }
  QString destFile = destPath + "/xlinuxtrack.xpl";
  QString newName;
  int counter = 0;
  if(QFile::exists(destFile)){
    do{
      newName = QString("/xlinuxtrack.xpl.") + QString::number(counter++);
    }while(QFile::exists(destPath + newName));
    if(!QFile::rename(destFile, destPath + newName)){
      warnMessage(QString("Couldn't rename ") + destFile + " to " + newName);
      return;
    }
  }
  if(!QFile::link(sourceFile, destFile)){
    warnMessage(QString("Couldn't link ") + sourceFile + " to " + destFile);
  }
}

