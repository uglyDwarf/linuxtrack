#define NEWS_SERIAL 1

#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include <QMessageBox>
#include <QTimer>
#include <QMutex>
#include <QMessageBox>
#include <QApplication>
#include <QDesktopWidget>
#include <QCursor>
#include "mickey.h"
#include "mouse.h"
#include "linuxtrack.h"
#include "piper.h"
#include "math_utils.h"
#include "transform.h"
#include <iostream>


//Time to wait after the tracking commences to perform a recentering [ms]
const int settleTime = 2000; //2 seconds


void RestrainWidgetToScreen(QWidget * w)
{
  QRect screenRect = QApplication::desktop()->availableGeometry(w);
  QRect wRect = w->frameGeometry();
/*    
  std::cout<<"fg left: "<<w->frameGeometry().left();
  std::cout<<" fg right: "<<w->frameGeometry().right();
  std::cout<<" fg top: "<<w->frameGeometry().top();
  std::cout<<" fg bottom: "<<w->frameGeometry().bottom()<<std::endl;
  
  std::cout<<"scr left: "<<screenRect.left();
  std::cout<<" scr right: "<<screenRect.right();
  std::cout<<" scr top: "<<screenRect.top();
  std::cout<<" scr bottom: "<<screenRect.bottom()<<std::endl;
*/  
  //make sure the window fits the screen
  if(screenRect.width() < wRect.width()){
    wRect.setWidth(screenRect.width());
  }
  if(screenRect.height() < wRect.height()){
    wRect.setHeight(screenRect.height());
  }
  //shuffle the window so it is fully visible
  if(screenRect.left() > wRect.left()){
    wRect.moveLeft(screenRect.left());
  }
  if(screenRect.right() < wRect.right()){
    wRect.moveRight(screenRect.right());
  }
  if(screenRect.bottom() < wRect.bottom()){
    wRect.moveBottom(screenRect.bottom());
  }
  if(screenRect.top() > wRect.top()){
    wRect.moveTop(screenRect.top());
  }
  w->resize(wRect.size());
  w->move(wRect.topLeft());
/*
  std::cout<<"fg left: "<<w->frameGeometry().left();
  std::cout<<" fg right: "<<w->frameGeometry().right();
  std::cout<<" fg top: "<<w->frameGeometry().top();
  std::cout<<" fg bottom: "<<w->frameGeometry().bottom()<<std::endl;
*/
}

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
  ui.FBString->setText(QString::fromUtf8("Will automatically revert back in %1 seconds...").arg(cntr));
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
    ui.FBString->setText(QString::fromUtf8("Will automatically revert back in %1 seconds...").arg(cntr));
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
        ui.CalibrationText->setText(QString::fromUtf8("Please center your head and sight."));
        ui.FBString->setText(QString::fromUtf8("Center position will be recorded in %1 seconds...").arg(cntr));
        break;
      case CALIBRATE:
        ui.CalibrationText->setText(QString::fromUtf8("Please move your head to left/right and up/down extremes."));
        ui.FBString->setText(QString::fromUtf8("Calibration will end in %1 seconds...").arg(cntr));
        break;
    }
}

mouseClass mouse = mouseClass();

MickeyThread::MickeyThread(Mickey *p) : QThread(p), fifo(-1), finish(false), parent(*p),
  fakeBtn(0)
{
}

void MickeyThread::processClick(sn4_btn_event_t ev)
{
  std::cout<<"Processing click! ("<<(int)ev.btns<<")"<<std::endl;
  int btns = ev.btns;
  mouse.click((buttons_t)btns, ev.timestamp);
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
      //std::cout<<"Thread alive!"<<std::endl;
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
  calDlg(), aplDlg(), recenterFlag(true), dw(NULL) 
{
  trans = new MickeyTransform();
  onOffSwitch = new shortcut();
  QObject::connect(onOffSwitch, SIGNAL(activated()), this, SLOT(onOffSwitch_activated()));
  //QObject::connect(&lbtnSwitch, SIGNAL(activated()), &btnThread, SLOT(on_key_pressed()));
  QObject::connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateTimer_activated()));
//  QObject::connect(&btnThread, SIGNAL(clicked()), this, SLOT(threadClicked()));
  QObject::connect(&aplDlg, SIGNAL(keep()), this, SLOT(keepSettings()));
  QObject::connect(&aplDlg, SIGNAL(revert()), this, SLOT(revertSettings()));

  QObject::connect(&calDlg, SIGNAL(recenterNow(bool)), this, SLOT(recenterNow(bool)));
  QObject::connect(&calDlg, SIGNAL(startCalibration()), this, SLOT(startCalibration()));
  QObject::connect(&calDlg, SIGNAL(finishCalibration()), this, SLOT(finishCalibration()));
  QObject::connect(&calDlg, SIGNAL(cancelCalibration(bool)), this, SLOT(cancelCalibration(bool)));
  
  if(!mouse.init()){
    exit(1);
  }
  dw = QApplication::desktop();
//  screenBBox = dw->screenGeometry();
  screenBBox = QRect(0, 0, dw->width(), dw->height());
  screenCenter = screenBBox.center();
  updateTimer.setSingleShot(false);
  updateTimer.setInterval(8);
  btnThread.start();
  linuxtrack_init((char *)"Mickey");
  changeState(TRACKING);
}

Mickey::~Mickey()
{
  if(linuxtrack_get_tracking_state() == RUNNING){
    linuxtrack_suspend();
  }
  updateTimer.stop();
  delete trans;
  trans = NULL;
}

void Mickey::screenResized(int screen)
{
  (void)screen;
//  screenBBox = dw->screenGeometry(screen);
  screenBBox = QRect(0, 0, dw->width(), dw->height());
  screenCenter = screenBBox.center();
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

void Mickey::keepSettings()
{
  trans->keepSettings();
}


void Mickey::pause()
{
  std::cout<<"Pausing!"<<std::endl;
  //btnThread.setFinish();
  //btnThread.wait();
  updateTimer.stop();
  //ltr_suspend();
}

void Mickey::wakeup()
{
  std::cout<<"Waking up!"<<std::endl;
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
      GUI.setStatusLabel(QString::fromUtf8("Tracking"));
      break;
    case CALIBRATING:
      GUI.setStatusLabel(QString::fromUtf8("Calibrating"));
      break;
    case STANDBY:
      GUI.setStatusLabel(QString::fromUtf8("Paused"));
      break;
  }
}

void Mickey::onOffSwitch_activated()
{
  std::cout<<"On/off switch activated!"<<std::endl;
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
  static int lastTrackingState = -1;
  int trackingState = linuxtrack_get_tracking_state();
  
  if(lastTrackingState != trackingState){
    lastTrackingState = trackingState;
    if(trackingState == RUNNING){
      switch(state){
        case TRACKING:
          GUI.setStatusLabel(QString::fromUtf8("Tracking"));
          break;
        case CALIBRATING:
          GUI.setStatusLabel(QString::fromUtf8("Calibrating"));
          break;
        case STANDBY:
          GUI.setStatusLabel(QString::fromUtf8("Paused"));
          break;
      }
    }else{
      switch(trackingState){
        case INITIALIZING:
          GUI.setStatusLabel(QString::fromUtf8("Initializing"));
          break;
        case ERROR:
          GUI.setStatusLabel(QString::fromUtf8("Error"));
          break;
        default:
          GUI.setStatusLabel(QString::fromUtf8("Inactive"));
          break;
      }
    }
  }
  
  if(trackingState != RUNNING){
    return;
  }
  if(recenterFlag){
    if(initTimer.elapsed() < settleTime){
      return;
    }else{
      linuxtrack_recenter();
      recenterFlag = false;
    }
  }
  if(linuxtrack_get_pose(&heading, &pitch, &roll, &tx, &ty, &tz, &counter) > 0){
    //new frame has arrived
    heading_p = heading;
    pitch_p = pitch;
    //ui.XLabel->setText(QString("X: %1").arg(heading));
    //ui.YLabel->setText(QString("Y: %1").arg(pitch));
  }
  int elapsed = updateElapsed.elapsed();
  updateElapsed.restart();
  //reversing signs to get the cursor move according to the head movement
  float dx, dy;
  trans->update(heading_p, pitch_p, relative, elapsed, dx, dy);
  if(state == TRACKING){
    if(relative){
      int idx = (int)dx;
      int idy = (int)dy;
      if((idx != 0) || (idy != 0)){
        mouse.move((int)dx, (int)dy);
        //QPoint pos = QCursor::pos();
        //pos += QPoint(dx,dy);
        //QCursor::setPos(pos);
      }
    }else{
      QPoint c = screenCenter + QPoint(screenCenter.x() * dx, screenCenter.y() * dy);
      QCursor::setPos(c);
    }
  }
}

void Mickey::startCalibration()
{
  trans->startCalibration();
}

void Mickey::finishCalibration()
{
  trans->finishCalibration();
  changeState(TRACKING);
  applySettings();
}

void Mickey::cancelCalibration(bool calStarted)
{
  if(calStarted){
    trans->cancelCalibration();
  }
  changeState(TRACKING);
}

bool Mickey::setShortcut(QKeySequence seq)
{
  bool res = onOffSwitch->setShortcut(seq);
  //If the shortcut doesn't work, pause the tracking to avoid problems
  if(res){
    changeState(TRACKING);
  }else{
    changeState(STANDBY);
  }
  return res;
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
  settings(QString::fromUtf8("linuxtrack"), QString::fromUtf8("mickey")), changed(false), hotkeySet(false)
{
  ui.setupUi(this);
  readPrefs();
  //mickey = new Mickey();
}

MickeyGUI::~MickeyGUI()
{
  delete mickey;
}

void MickeyGUI::readPrefs()
{
  //Read hotkey setup
  QString modifier, key;
  
  settings.beginGroup(QString::fromUtf8("Shortcut"));
  modifier = settings.value(QString::fromUtf8("Modifier"), QString::fromUtf8("None")).toString();
  key = settings.value(QString::fromUtf8("Key"), QString::fromUtf8("F9")).toString();
  settings.endGroup();
  
  modifierIndex = ui.ModifierCombo->findText(modifier);
  if(modifierIndex != -1){
    ui.ModifierCombo->setCurrentIndex(modifierIndex);
  }
  hotkeyIndex = ui.KeyCombo->findText(key);
  if(hotkeyIndex != -1){
    ui.KeyCombo->setCurrentIndex(hotkeyIndex);
  }
  
  //Axes setup
  bool relative;
  settings.beginGroup(QString::fromUtf8("Axes"));
  deadzone = settings.value(QString::fromUtf8("DeadZone"), 20).toInt();
  sensitivity = settings.value(QString::fromUtf8("Sensitivity"), 50).toInt();
  curvature = settings.value(QString::fromUtf8("Curvature"), 50).toInt();
  stepOnly = settings.value(QString::fromUtf8("StepOnly"), false).toBool();
  relative = settings.value(QString::fromUtf8("Relative"), true).toBool();
  ui.SensSlider->setValue(sensitivity);
  ui.DZSlider->setValue(deadzone);
  ui.CurveSlider->setValue(curvature);
  ui.StepOnly->setCheckState(stepOnly ? Qt::Checked : Qt::Unchecked);
  if(stepOnly){
    ui.CurveSlider->setDisabled(true);
  }else{
    ui.CurveSlider->setEnabled(true);
  }
  if(relative){
    ui.RelativeCB->setChecked(true);
  }else{
    ui.AbsoluteCB->setChecked(true);
    ui.SensSlider->setDisabled(true);
    ui.DZSlider->setDisabled(true);
    ui.CurveSlider->setDisabled(true);
    ui.StepOnly->setDisabled(true);
  }
  settings.endGroup();
  
  //trans setup
  settings.beginGroup(QString::fromUtf8("Transform"));
  maxValX = settings.value(QString::fromUtf8("RangeX"), 130).toFloat();
  maxValY = settings.value(QString::fromUtf8("RangeY"), 130).toFloat();
  settings.endGroup();
  
  //calibration setup
  settings.beginGroup(QString::fromUtf8("Calibration"));
  calDelay = settings.value(QString::fromUtf8("CalibrationDelay"), 10).toInt();
  cntrDelay = settings.value(QString::fromUtf8("CenteringDelay"), 10).toInt();
  settings.endGroup();
  ui.CalibrationTimeout->setValue(calDelay);
  ui.CenterTimeout->setValue(cntrDelay);

  HelpViewer::LoadPrefs(settings);

  settings.beginGroup(QString::fromUtf8("Misc"));
  welcome = settings.value(QString::fromUtf8("welcome"), true).toBool();
  newsSerial = settings.value(QString::fromUtf8("news"), -1).toInt();
  resize(settings.value(QString::fromUtf8("size"), QSize(-1, -1)).toSize());
  move(settings.value(QString::fromUtf8("pos"), QPoint(0, 0)).toPoint());
  settings.endGroup();
}

void MickeyGUI::storePrefs()
{
  HelpViewer::StorePrefs(settings);
  
  settings.beginGroup(QString::fromUtf8("Misc"));
  settings.setValue(QString::fromUtf8("welcome"), false);
  settings.setValue(QString::fromUtf8("news"), NEWS_SERIAL);
  settings.setValue(QString::fromUtf8("size"), size());
  settings.setValue(QString::fromUtf8("pos"), pos());
  settings.endGroup();

  if(!changed){
    return;
  }
  if(QMessageBox::question(this, QString::fromUtf8("Save prefs?"), 
    QString::fromUtf8("Preferences have changed, do you want to save them?"), 
    QMessageBox::Save | QMessageBox::Discard, QMessageBox::Save) == QMessageBox::Discard){
    return;  
  }  
  
  //Store hotkey setup
  settings.beginGroup(QString::fromUtf8("Shortcut"));
  settings.setValue(QString::fromUtf8("Modifier"), ui.ModifierCombo->currentText());
  settings.setValue(QString::fromUtf8("Key"), ui.KeyCombo->currentText());
  settings.endGroup();
  
  //Axes setup
  settings.beginGroup(QString::fromUtf8("Axes"));
  settings.setValue(QString::fromUtf8("DeadZone"), deadzone);
  settings.setValue(QString::fromUtf8("Sensitivity"), sensitivity);
  settings.setValue(QString::fromUtf8("Curvature"), curvature);
  settings.setValue(QString::fromUtf8("StepOnly"), stepOnly);
  settings.setValue(QString::fromUtf8("Relative"), mickey->getRelative());
  settings.endGroup();
  
  //trans setup
  settings.beginGroup(QString::fromUtf8("Transform"));
  settings.setValue(QString::fromUtf8("RangeX"), maxValX);
  settings.setValue(QString::fromUtf8("RangeY"), maxValY);
  settings.endGroup();

  //calibration setup
  settings.beginGroup(QString::fromUtf8("Calibration"));
  settings.setValue(QString::fromUtf8("CalibrationDelay"), calDelay);
  settings.setValue(QString::fromUtf8("CenteringDelay"), cntrDelay);
  settings.endGroup();
}

void MickeyGUI::setStepOnly(bool value)
{
  changed = true;
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

bool MickeyGUI::getShortcut()
{
  if(mickey == NULL){
    return false;
  }
  changed = true;
  
  QString modifier(QString::fromUtf8(""));
  if(ui.ModifierCombo->currentIndex() != 0){
    modifier = ui.ModifierCombo->currentText() + QString::fromUtf8("+");
  }
  modifier += ui.KeyCombo->currentText();
  QKeySequence seq(modifier);
  if(mickey->setShortcut(seq)){
    std::cout<<"Shortcut set!"<<std::endl;
    modifierIndex = ui.ModifierCombo->currentIndex();
    hotkeyIndex = ui.KeyCombo->currentIndex();
    return true;
  }else{
    QMessageBox::warning(this, QString::fromUtf8("Hotkey not usable."), 
      QString::fromUtf8("The hotkey you set is already in use! Please select another one."));
    //try to revert shortcut, if it was working before...
    if(hotkeySet){
      //this should avoid recursion
      hotkeySet = false;
      ui.ModifierCombo->setCurrentIndex(modifierIndex);
      ui.KeyCombo->setCurrentIndex(hotkeyIndex);
      return getShortcut();
    }
  }
  return false;
}

void MickeyGUI::closeEvent(QCloseEvent *event)
{
  storePrefs();
  HelpViewer::CloseWindow();
  QWidget::closeEvent(event);
}

void MickeyGUI::on_MickeyTabs_currentChanged(int index)
{
  switch(index){
    case 0:
      HelpViewer::ChangePage(QString::fromUtf8("tracking.htm"));
      break;
    case 1:
      HelpViewer::ChangePage(QString::fromUtf8("misc.htm"));
      break;
  }
}

void MickeyGUI::show()
{
  QWidget::show();
  setWindowTitle(QString::fromUtf8("Mickey v")+QString::fromUtf8(PACKAGE_VERSION));
  RestrainWidgetToScreen(this);
  if(welcome){
    HelpViewer::ChangePage(QString::fromUtf8("welcome.htm"));
    HelpViewer::ShowWindow();
  }else if(newsSerial < NEWS_SERIAL){
    HelpViewer::ChangePage(QString::fromUtf8("news.htm"));
    HelpViewer::ShowWindow();
  }else{
    HelpViewer::ChangePage(QString::fromUtf8("tracking.htm"));
  }
}

//To avoid recursion as Mickey's constructor uses GUI too
void MickeyGUI::init()
{
  mickey = new Mickey();

  ui.ApplyButton->setEnabled(false);
  mickey->setRelative(ui.RelativeCB->isChecked());
  hotkeySet = getShortcut();
  changed = false;
}
  
