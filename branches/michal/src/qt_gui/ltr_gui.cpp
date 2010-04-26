#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
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
  sc->setSlaves(ui.PitchEnable, ui.PitchUpSpin, ui.PitchDownSpin,
                ui.RollEnable, ui.TiltLeftSpin, ui.TiltRightSpin,
                ui.YawEnable, ui.YawLeftSpin, ui.YawRightSpin,
                ui.XEnable, ui.MoveLeftSpin, ui.MoveRightSpin,
                ui.YEnable, ui.MoveUpSpin, ui.MoveDownSpin,
                ui.ZEnable, ui.MoveBackSpin, ui.MoveForthSpin
                );
  QObject::connect(this, SIGNAL(customSectionChanged()), sc, SLOT(reinit()));
  showWindow = new LtrGuiForm(sc);
  initFilterFactor();
  ui.Profiles->addItems(Profiles::getProfiles().getProfileNames());
  helper = new LtrDevHelp(sc);
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
  delete sc;
  delete helper;
}

void LinuxtrackGui::closeEvent(QCloseEvent *event)
{
  showWindow->close();
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

void LinuxtrackGui::initFilterFactor()
{
  QString val;
  PREF.getKeyVal("Filter-factor", val);
  float f = val.toFloat();
  ui.FilterSlider->setValue(f * 10);
}

void LinuxtrackGui::on_FilterSlider_valueChanged(int value)
{
  ui.FilterValue->setText(QString::number(value / 10.0f));
  PREF.setKeyVal(Profiles::getProfiles().getCurrent(), "Filter-factor", value / 10.0f);
}

Profiles::Profiles()
{
  PREF.getProfiles(names);
  names.prepend("Default");
  current = "Default";
}

Profiles *Profiles::profs = NULL;

Profiles& Profiles::getProfiles()
{
  if(profs == NULL){
    profs = new Profiles();
  }
  return *profs;
}

void Profiles::addProfile(const QString &name)
{
  names.append(name);
}

const QStringList &Profiles::getProfileNames()
{
  return names;
}

bool Profiles::setCurrent(const QString &name)
{
  if(!names.contains(name, Qt::CaseInsensitive)){
    return false;
  }
  current = name;
  return true;
}

const QString &Profiles::getCurrent()
{
  return current;
}

int Profiles::isProfile(const QString &name)
{
  int i = -1;
  if(names.contains(name, Qt::CaseInsensitive)){
    for(i = 0; i < names.size(); ++i){
      if(names[i].compare(name, Qt::CaseInsensitive) == 0){
	break;
      }
    }
  }
  return i;
}

void LinuxtrackGui::on_Profiles_currentIndexChanged(const QString &text)
{
  Profiles::getProfiles().setCurrent(text);
  PREF.setCustomSection(text);
  initFilterFactor();
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
    int i = Profiles::getProfiles().isProfile(newSec);
    if(i == -1){
      QString section_name = "Profile";
      PREF.createSection(section_name);
      PREF.addKeyVal(section_name, "Title", newSec);
      Profiles::getProfiles().addProfile(newSec);
      ui.Profiles->clear();
      const QStringList &sl = Profiles::getProfiles().getProfileNames();
      ui.Profiles->addItems(sl);
      ui.Profiles->setCurrentIndex(sl.size() - 1);
    }else{
      ui.Profiles->setCurrentIndex(i);
    }
  }
}