#include <QMessageBox>
#include <QTimer>
#include <QMutex>
#include "mickey.h"
#include "uinput_ifc.h"
#include "linuxtrack.h"
#include "piper.h"
#include "math_utils.h"
#include "transform.h"
#include <iostream>


//Time to wait after the tracking commences to perform a recentering [ms]
const int settleTime = 2000; //2 seconds

QMutex MickeyUinput::mutex;


MickeyApplyDialog::MickeyApplyDialog()
{
  ui.setupUi(this);
  timer.setSingleShot(false);
  QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(timeout()));
  setModal(true);
}

void MickeyApplyDialog::closeEvent(QCloseEvent *event)
{
  timer.stop();
  emit keep();
  QDialog::closeEvent(event);
}


void MickeyApplyDialog::trySettings()
{
  timer.start(1000);
  cntr = 15;
  ui.FBString->setText(QString("Will automatically revert back in %1 seconds...").arg(cntr));
  show();
  raise();
  activateWindow();
  std::cout<<"trying settings!"<<std::endl;
}

void MickeyApplyDialog::timeout()
{
  std::cout<<"Apl timeout!"<<std::endl;
  --cntr;
  if(cntr == 0){
    timer.stop();
    emit revert();
    hide();
  }else{
    ui.FBString->setText(QString("Will automatically revert back in %1 seconds...").arg(cntr));
  }
}

void MickeyApplyDialog::on_RevertButton_pressed()
{
  timer.stop();
  std::cout<<"Reverting..."<<std::endl;
  emit revert();
  hide();
}

void MickeyApplyDialog::on_KeepButton_pressed()
{
  timer.stop();
  std::cout<<"Keeping..."<<std::endl;
  emit keep();
  hide();
}

MickeyCalibration::MickeyCalibration()
{
  ui.setupUi(this);
  timer.setSingleShot(false);
  QObject::connect(ui.CancelButton, SIGNAL(pressed()), this, SLOT(cancelPressed()));
  QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(timeout()));
  setModal(true);
}

void MickeyCalibration::closeEvent(QCloseEvent *event)
{
  cancelPressed();
  QDialog::closeEvent(event);
}

/*
void MickeyCalibration::hide()
{
  std::cout<<"Hide called!"<<std::endl;
  timer.stop();
  if(calState == CALIBRATE){
    emit cancelCalibration();
  }
  QWidget::hide();
}
*/
void MickeyCalibration::calibrate()
{
  timer.start(1000);
  calState = CENTER;
  cntr = GUI.getCntrDelay() + 1;
  timeout();
  window()->show();
  window()->raise();
  window()->activateWindow();
}

void MickeyCalibration::recenter()
{
  timer.start(1000);
  calState = CENTER_ONLY;
  cntr = GUI.getCntrDelay() + 1;// +1 - compensation for first increment in timeout()
  timeout();
  window()->show();
  window()->raise();
  window()->activateWindow();
}

void MickeyCalibration::cancelPressed()
{
  timer.stop();
  if(calState == CALIBRATE){
    emit cancelCalibration(true);
  }else{
    emit cancelCalibration(false);
  }
  hide();
}

void MickeyCalibration::timeout()
{
  std::cout<<"timer!"<<std::endl;
  --cntr;
  if(cntr == 0){
    switch(calState){
      case CENTER_ONLY:
        timer.stop();
        emit recenterNow(true);
        window()->hide();
        break;
      case CENTER:
        emit recenterNow(false);
        emit startCalibration();
        calState = CALIBRATE;
        cntr = GUI.getCalDelay();
        break;
      case CALIBRATE:
        timer.stop();
        emit finishCalibration();
        window()->hide();
        break;
    }
  }
    switch(calState){
      case CENTER:
      case CENTER_ONLY:
        ui.CalibrationText->setText(QString("Please center your head and sight."));
        ui.FBString->setText(QString("Center position will be recorded in %1 seconds...").arg(cntr));
        break;
      case CALIBRATE:
        ui.CalibrationText->setText(QString("Please move your head to left/right and up/down extremes."));
        ui.FBString->setText(QString("Calibration will end in %1 seconds...").arg(cntr));
        break;
    }
}

MickeyUinput::MickeyUinput() : fd(-1)
{
}

MickeyUinput::~MickeyUinput()
{
  close_uinput(fd);
  fd = -1;
}

bool MickeyUinput::init()
{
  char *fname = NULL;
  bool permProblem;
  fd = open_uinput(&fname, &permProblem);
  if(fd > 0){
    return create_device(fd);
  }else{
    QMessageBox::critical(NULL, "Error Creating Virtual Mouse", 
      QString("There was a problem accessing the file \"%1\"\n\
 Please check that you have the right to write to it.").arg(fname));
    return false;
  }

}

void MickeyUinput::mouseClick(sn4_btn_event_t ev)
{
  mutex.lock();
  click(fd, ev.btns, ev.timestamp);
  mutex.unlock();
}

void MickeyUinput::mouseMove(int dx, int dy)
{
  mutex.lock();
  movem(fd, dx, dy);
  mutex.unlock();
}

MickeyUinput uinput = MickeyUinput();

MickeyThread::MickeyThread(Mickey *p) : QThread(p), fifo(-1), finish(false), parent(*p),
  fakeBtn(0)
{
}

void MickeyThread::processClick(sn4_btn_event_t ev)
{
  std::cout<<"Processing click! ("<<(int)ev.btns<<")"<<std::endl;
    uinput.mouseClick(ev);
}

//emulate mouse button press using keyboard
void MickeyThread::on_key_pressed()
{
  std::cout<<"Button pressed!!!"<<std::endl;
  fakeBtn ^= 1;
  sn4_btn_event_t ev;
  ev.btns = fakeBtn;
  gettimeofday(&(ev.timestamp), NULL);
  processClick(ev);
}



void MickeyThread::run()
{
  sn4_btn_event_t ev;
  ssize_t read;
  finish = false;
  while(1){
    while(fifo <= 1){
      if(finish){
        return;
      }
      fifo = prepareBtnChanel();
      if(fifo <= 0){
        QThread::sleep(1);
      }
    }
    while(fetch_data(fifo, (void*)&ev, sizeof(ev), &read)){
      std::cout<<"Thread alive!"<<std::endl;
      if(finish){
        return;
      }
      if(read == sizeof(ev)){
        ev.btns ^= 3;
        processClick(ev);
      }
    }
  }
}
  


Mickey::Mickey() : updateTimer(this), btnThread(this), state(STANDBY), 
  calDlg(), aplDlg(), recenterFlag(true)
{
  trans = new MickeyTransform();
  onOffSwitch = new shortcut();
  QObject::connect(onOffSwitch, SIGNAL(activated()), this, SLOT(onOffSwitch_activated()));
  //QObject::connect(&lbtnSwitch, SIGNAL(activated()), &btnThread, SLOT(on_key_pressed()));
  QObject::connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateTimer_activated()));
//  QObject::connect(&btnThread, SIGNAL(clicked()), this, SLOT(threadClicked()));
  QObject::connect(&aplDlg, SIGNAL(keep()), this, SLOT(keepSettings()));
  QObject::connect(&aplDlg, SIGNAL(revert()), this, SLOT(revertSettings()));

  QObject::connect(&calDlg, SIGNAL(recenterNow()), this, SLOT(recenterNow()));
  QObject::connect(&calDlg, SIGNAL(startCalibration()), this, SLOT(startCalibration()));
  QObject::connect(&calDlg, SIGNAL(finishCalibration()), this, SLOT(finishCalibration()));
  QObject::connect(&calDlg, SIGNAL(cancelCalibration()), this, SLOT(cancelCalibration()));
  
  if(!uinput.init()){
    exit(1);
  }
  updateTimer.setSingleShot(false);
  updateTimer.setInterval(8);
  ltr_init((char *)"Mickey");
  changeState(TRACKING);
}

Mickey::~Mickey()
{
  ltr_suspend();
  updateTimer.stop();
  delete trans;
  trans = NULL;
}

void Mickey::applySettings()
{
  aplDlg.trySettings();
  trans->applySettings();
}

void Mickey::revertSettings()
{
  trans->revertSettings();
}


void Mickey::pause()
{
  //btnThread.setFinish();
  //btnThread.wait();
  updateTimer.stop();
  //ltr_suspend();
}

void Mickey::wakeup()
{
  updateTimer.start();
  //btnThread.start();
  //ltr_wakeup();
  //ltr_recenter();
}

void Mickey::calibrate()
{
  changeState(CALIBRATING);
  calDlg.calibrate();
}

void Mickey::recenter()
{
  changeState(CALIBRATING);
  calDlg.recenter();
}


void Mickey::changeState(state_t newState)
{
  switch(state){
    case TRACKING:
      switch(newState){
        case TRACKING:
          //nothing to do
          break;
        case STANDBY:
          pause();
          break;
        case CALIBRATING:
          startCalibration();
          break;
      }
      break;
    case STANDBY:
      switch(newState){
        case TRACKING:
          wakeup();
          break;
        case STANDBY:
          //nothing to do
          break;
        case CALIBRATING:
          wakeup();
          startCalibration();
          break;
      }
      break;
    case CALIBRATING:
      switch(newState){
        case TRACKING:
          //calDlg.hide();
          //just continue...
          break;
        case STANDBY:
          //don't let this happen
          newState = CALIBRATING;
          break;
        case CALIBRATING:
          //nothing to do
          break;
      }
      break;
  }
  state = newState;
  switch(newState){
    case TRACKING:
      GUI.setStatusLabel("Tracking");
      break;
    case CALIBRATING:
      GUI.setStatusLabel("Calibrating");
      break;
    case STANDBY:
      GUI.setStatusLabel("Paused");
      break;
  }
}

void Mickey::onOffSwitch_activated()
{
  switch(state){
    case TRACKING:
      changeState(STANDBY);
      break;
    case STANDBY:
      changeState(TRACKING);
      initTimer.start();
      break;
    default:
      break;
  }
}


void Mickey::updateTimer_activated()
{
  static float heading_p = 0.0;
  static float pitch_p = 0.0;
  float heading, pitch, roll, tx, ty, tz;
  unsigned int counter;
  static unsigned int last_counter = 0;
  if(ltr_get_tracking_state() != RUNNING){
    return;
  }
  if(recenterFlag){
    if(initTimer.elapsed() < settleTime){
      return;
    }else{
      ltr_recenter();
      recenterFlag = false;
    }
  }
  if(ltr_get_camera_update(&heading, &pitch, &roll, &tx, &ty, &tz, &counter) == 0){
    if(counter != last_counter){
      //new frame has arrived
      last_counter = counter;
      heading_p = heading;
      pitch_p = pitch;
      //ui.XLabel->setText(QString("X: %1").arg(heading));
      //ui.YLabel->setText(QString("Y: %1").arg(pitch));
    }
  }
  int elapsed = updateElapsed.elapsed();
  updateElapsed.restart();
  //reversing signs to get the cursor move according to the head movement
  int dx, dy;
  trans->update(heading_p, pitch_p, elapsed, dx, dy);
  if(state == TRACKING){
    uinput.mouseMove(dx, dy);
  }
}

/*
void Mickey::threadClicked()
{
  std::cout<<"thread clicked!!!"<<state<<":"<<calState<<std::endl;
  if(state == CALIBRATING){
    switch(calState){
      case CENTER:
        std::cout<<"Centering..."<<std::endl;
        ltr_recenter();
        calState = CALIBRATE;
        trans->startCalibration();
        calDlg.setText((char *)"Move your head to all extremes...");
        break;
      case CALIBRATE:
        std::cout<<"Calibration finished..."<<std::endl;
        trans->finishCalibration();
        changeState(TRACKING);
        cdg.hide();
        break;
      default:
        std::cout<<"Whatever..."<<std::endl;
        cdg.hide();
        calState = CENTER;
        break;
    }
  }else{
    changeState(TRACKING);
  }
}
*/

void Mickey::startCalibration()
{
  trans->startCalibration();
}

void Mickey::finishCalibration()
{
  trans->finishCalibration();
}

void Mickey::cancelCalibration(bool calStarted)
{
  if(calStarted){
    trans->cancelCalibration();
  }
  changeState(TRACKING);
}

MickeyGUI *MickeyGUI::instance = NULL;

MickeyGUI &MickeyGUI::getInstance()
{
  if(instance == NULL){
    instance = new MickeyGUI();
    instance->init();
  }
  return *instance;
}

void MickeyGUI::deleteInstance()
{
  if(instance != NULL){
    MickeyGUI *tmp = instance;
    instance = NULL;
    delete tmp;
  }
}

MickeyGUI::MickeyGUI(QWidget *parent) : QWidget(parent), mickey(NULL), 
  settings("linuxtrack", "mickey")
{
  ui.setupUi(this);
  readPrefs();
  ui.ApplyButton->setEnabled(false);
  //mickey = new Mickey();
}

MickeyGUI::~MickeyGUI()
{
  delete mickey;
}

void MickeyGUI::readPrefs()
{
  //Read hotkey setup
  int tmp;
  QString modifier, key;
  
  settings.beginGroup("Shortcut");
  modifier = settings.value(QString("Modifier"), "None").toString();
  key = settings.value(QString("Key"), "F9").toString();
  settings.endGroup();
  
  tmp = ui.ModifierCombo->findText(modifier);
  if(tmp != -1){
    ui.ModifierCombo->setCurrentIndex(tmp);
  }
  tmp = ui.KeyCombo->findText(key);
  if(tmp != -1){
    ui.KeyCombo->setCurrentIndex(tmp);
  }
  
  //Axes setup
  settings.beginGroup("Axes");
  deadzone = settings.value(QString("DeadZone"), 20).toInt();
  sensitivity = settings.value(QString("Sensitivity"), 50).toInt();
  curvature = settings.value(QString("Curvature"), 50).toInt();
  stepOnly = settings.value(QString("StepOnly"), false).toBool();
  ui.SensSlider->setValue(sensitivity);
  ui.DZSlider->setValue(deadzone);
  ui.CurveSlider->setValue(curvature);
  ui.StepOnly->setCheckState(stepOnly ? Qt::Checked : Qt::Unchecked);
  if(stepOnly){
    ui.CurveSlider->setDisabled(true);
  }else{
    ui.CurveSlider->setEnabled(true);
  }
  settings.endGroup();
  
  //trans setup
  settings.beginGroup("Transform");
  maxValX = settings.value(QString("RangeX"), 130).toFloat();
  maxValY = settings.value(QString("RangeY"), 130).toFloat();
  settings.endGroup();
  
  //calibration setup
  settings.beginGroup("Calibration");
  calDelay = settings.value(QString("CalibrationDelay"), 10).toInt();
  cntrDelay = settings.value(QString("CenteringDelay"), 10).toInt();
  settings.endGroup();
  ui.CalibrationTimeout->setValue(calDelay);
  ui.CenterTimeout->setValue(cntrDelay);
}

void MickeyGUI::storePrefs()
{
  //Store hotkey setup
  settings.beginGroup("Shortcut");
  settings.setValue(QString("Modifier"), ui.ModifierCombo->currentText());
  settings.setValue(QString("Key"), ui.KeyCombo->currentText());
  settings.endGroup();
  
  //Axes setup
  settings.beginGroup("Axes");
  settings.setValue(QString("DeadZone"), deadzone);
  settings.setValue(QString("Sensitivity"), sensitivity);
  settings.setValue(QString("Curvature"), curvature);
  settings.setValue(QString("StepOnly"), stepOnly);
  settings.endGroup();
  
  //trans setup
  settings.beginGroup("Transform");
  settings.setValue(QString("RangeX"), maxValX);
  settings.setValue(QString("RangeY"), maxValY);
  settings.endGroup();

  //calibration setup
  settings.beginGroup("Calibration");
  settings.setValue(QString("CalibrationDelay"), calDelay);
  settings.setValue(QString("CenteringDelay"), cntrDelay);
  settings.endGroup();
}

void MickeyGUI::setStepOnly(bool value)
{
  stepOnly = value;
  ui.StepOnly->setCheckState(stepOnly ? Qt::Checked : Qt::Unchecked);
  if(stepOnly){
    ui.CurveSlider->setDisabled(true);
  }else{
    ui.CurveSlider->setEnabled(true);
  }
}

void MickeyGUI::on_StepOnly_stateChanged(int state)
{
  stepOnly = (state == Qt::Checked); 
  if(stepOnly){
    ui.CurveSlider->setDisabled(true);
  }else{
    ui.CurveSlider->setEnabled(true);
  }
  emit axisChanged();
  ui.ApplyButton->setEnabled(true);
}

void MickeyGUI::getShortcut()
{
  if(mickey == NULL){
    return;
  }
  QString modifier("");
  if(ui.ModifierCombo->currentIndex() != 0){
    modifier = ui.ModifierCombo->currentText() + QString("+");
  }
  modifier += ui.KeyCombo->currentText();
  QKeySequence seq(modifier);
  if(mickey->setShortcut(seq)){
    std::cout<<"Shortcut OK!"<<std::endl;
  }else{
    std::cout<<"Shortcut not set!"<<std::endl;
  }
}

void MickeyGUI::closeEvent(QCloseEvent *event)
{
  storePrefs();
  QWidget::closeEvent(event);
}