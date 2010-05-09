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
#include "ltr_profiles.h"

LinuxtrackGui::LinuxtrackGui(QWidget *parent) : QWidget(parent)
{
  QString target = QDir::homePath() + "/.linuxtrack";
  if(!QFile::exists(target)){
    QString source = QString(PREFIX) + "/share/linuxtrack/.linuxtrack";
    std::cout<<"Going to copy "<<source.toAscii().data();
    std::cout<<" to "<<target.toAscii().data()<<std::endl;
    QFile::copy(source, target);
  }
  ui.setupUi(this);
  wcp = new WebcamPrefs(ui);
  wiip = new WiimotePrefs(ui);
  tirp = new TirPrefs(ui);
  me = new ModelEdit(ui);
//  sc = new ScpForm();
  lv = new LogView();
/*  
  sc->setSlaves(ui.PitchEnable, ui.PitchUpSpin, ui.PitchDownSpin,
                ui.RollEnable, ui.TiltLeftSpin, ui.TiltRightSpin,
                ui.YawEnable, ui.YawLeftSpin, ui.YawRightSpin,
                ui.XEnable, ui.MoveLeftSpin, ui.MoveRightSpin,
                ui.YEnable, ui.MoveUpSpin, ui.MoveDownSpin,
                ui.ZEnable, ui.MoveBackSpin, ui.MoveForthSpin
                );
*/
//  QObject::connect(this, SIGNAL(customSectionChanged()), sc, SLOT(reinit()));
  
  QObject::connect(&STATE, SIGNAL(trackerStopped()), this, SLOT(trackerStopped()));
  QObject::connect(&STATE, SIGNAL(trackerRunning()), this, SLOT(trackerRunning()));
  
  ffChanged(PROFILE.getCurrentProfile()->getFilterFactor());
  QObject::connect(PROFILE.getCurrentProfile(), 
                    SIGNAL(filterFactorChanged(float)),this, SLOT(ffChanged(float)));
  
//  showWindow = new LtrGuiForm(sc);
  showWindow = new LtrGuiForm();
  ui.Profiles->addItems(Profile::getProfiles().getProfileNames());
  helper = new LtrDevHelp();
  on_RefreshDevices_pressed();
  showWindow->show();
  helper->show();
}

LinuxtrackGui::~LinuxtrackGui()
{
  delete showWindow;
  delete wcp;
  delete wiip;
  delete tirp;
  delete me;
//  delete sc;
  delete helper;
}

void LinuxtrackGui::closeEvent(QCloseEvent *event)
{
  showWindow->close();
  helper->close();
//  sc->close();
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
//  sc->show();
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

void LinuxtrackGui::ffChanged(float f)
{
  ui.FilterValue->setText(QString::number(f));
  ui.FilterSlider->setValue(f * 10 + 1);
}

void LinuxtrackGui::on_FilterSlider_valueChanged(int value)
{
  ui.FilterValue->setText(QString::number(value / 10.0f));
  PROFILE.getCurrentProfile()->setFilterFactor(value / 10.0f);
}

void LinuxtrackGui::on_SaveButton_pressed()
{
  PREF.savePrefs();
}

void LinuxtrackGui::on_ViewLogButton_pressed()
{
  lv->show();
}

void LinuxtrackGui::trackerStopped()
{
  ui.DeviceSelector->setEnabled(true);
  ui.ModelSelector->setEnabled(true);
  ui.Profiles->setEnabled(true);
}

void LinuxtrackGui::trackerRunning()
{
  ui.DeviceSelector->setDisabled(true);
  ui.ModelSelector->setDisabled(true);
  ui.Profiles->setDisabled(true);
}

void LinuxtrackGui::on_Profiles_currentIndexChanged(const QString &text)
{
  PROFILE.setCurrent(text);
  emit customSectionChanged();
}

void LinuxtrackGui::on_CreateNewProfile_pressed()
{
  bool done;
  QString newSec;
  newSec = QInputDialog::getText(NULL, "New Secion Name:", 
		        "Enter name of the new section:", 
			QLineEdit::Normal, "", &done);
  if(done && !newSec.isEmpty()){
    int i = PROFILE.isProfile(newSec);
    if(i == -1){
      PROFILE.addProfile(newSec);
      ui.Profiles->clear();
      const QStringList &sl = Profile::getProfiles().getProfileNames();
      ui.Profiles->addItems(sl);
      ui.Profiles->setCurrentIndex(sl.size() - 1);
    }else{
      ui.Profiles->setCurrentIndex(i);
    }
  }
}