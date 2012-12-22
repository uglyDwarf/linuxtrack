#include <QMessageBox>
#include <QTimer>
#include <QMutex>
#include "mickey.h"
#include "uinput_ifc.h"
#include "linuxtrack.h"
#include "piper.h"
#include "math_utils.h"
#include <iostream>


//Time to wait after the tracking commences to perform a recentering [ms]
const int settleTime = 2000; //2 seconds
const int screenMax = 1024;
const float timeFast = 0.1;
const float timeSlow = 4;

QMutex MickeyUinput::mutex;

MickeyApplyDialog::MickeyApplyDialog(QWidget *parent) : QWidget(parent)
{
  ui.setupUi(this);
  timer.setSingleShot(false);
  QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(timeout()));
}

void MickeyApplyDialog::trySettings()
{
  timer.start(1000);
  cntr = 15;
  ui.FBString->setText(QString("Will automatically revert back in %1 seconds...").arg(cntr));
  window()->show();
  window()->raise();
  window()->activateWindow();
  std::cout<<"trying settings!"<<std::endl;
}

void MickeyApplyDialog::timeout()
{
  --cntr;
  if(cntr == 0){
    timer.stop();
    emit revert();
    window()->hide();
  }else{
    ui.FBString->setText(QString("Will automatically revert back in %1 seconds...").arg(cntr));
  }
}

void MickeyApplyDialog::on_RevertButton_pressed()
{
  std::cout<<"Reverting..."<<std::endl;
  emit revert();
  window()->hide();
}

void MickeyApplyDialog::on_KeepButton_pressed()
{
  std::cout<<"Keeping..."<<std::endl;
  emit keep();
  window()->hide();
}

MickeyCalibration::MickeyCalibration(QWidget *parent): QWidget(parent)
{
  ui.setupUi(this);
  QObject::connect(ui.NextButton, SIGNAL(pressed()), this, SIGNAL(nextClicked()));
  QObject::connect(ui.CancelButton, SIGNAL(pressed()), this, SIGNAL(cancelClicked()));
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
  //static int cal_off;
  state_t state = parent.getState();
  if(state == TRACKING){
    uinput.mouseClick(ev);
  }else if(state == CALIBRATING){
    emit clicked();
  }
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
  
  
//Deadzone - 0 - 50%...
//Sensitivity - normalize input, apply curve, return (-1, 1); outside - convert to speed...
//Dependency on updte rate...


MickeysAxis::MickeysAxis() : deadZone(0), sensitivity(0),
  accX(0.0), accY(0.0),settings("linuxtrack", "mickey"), calibrating(false)
{
  settings.beginGroup("Axes");
  deadZone = settings.value(QString("DeadZone"), 20).toInt();
  sensitivity = settings.value(QString("Sensitivity"), 50).toInt();
  maxVal = settings.value(QString("Range"), 130).toFloat();
  std::cout<<"DZ: "<<deadZone<<std::endl;
  std::cout<<"Sensitivity: "<<sensitivity<<std::endl;
  std::cout<<"Range: "<<maxVal<<std::endl;
  settings.endGroup();
}

MickeysAxis::~MickeysAxis()
{
  settings.beginGroup("Axes");
  settings.setValue(QString("DeadZone"), deadZone);
  settings.setValue(QString("Sensitivity"), sensitivity);
  settings.setValue(QString("Range"), maxVal);
  settings.endGroup();
}

void MickeysAxis::changeDeadZone(int dz)
{
  deadZone = dz;
}

void MickeysAxis::changeSensitivity(int sens)
{
  sensitivity = sens;
}

static float sign(float val)
{
  if(val >= 0) return 1.0;
  return -1.0;
}

void MickeysAxis::processValue(float valX, float valY, int elapsed, float &accX, float &accY)
{
  //std::cout<<qPrintable(identificator)<<": "<<val<<" => ";
  valX /= maxVal; //normalize the value
  valY /= maxVal; //normalize the value
  //std::cout<<qPrintable(identificator)<<": "<<val<<" => ";
  float mag = sqrtf(valX * valX + valY * valY);
  float angle = atan2f(valY, valX);
  if(mag > 1) mag = 1;
  //deadzone 0 - 50% of the maxValue
  float dz = 0.5 * ((float)deadZone) / 99.0f;
  if(mag > dz){
    mag = 1;
  }else{
    mag = 0;
  }
  accX += mag * cosf(angle) * getSpeed(sensitivity) * (elapsed / 1000.0);
  accY += mag * sinf(angle) * getSpeed(sensitivity) * (elapsed / 1000.0);
  
  //std::cout<<val<<std::endl;
}

void MickeysAxis::updateAxis(float valX, float valY, int elapsed, int &x, int &y)
{
  if(!calibrating){
    processValue(valX, valY, elapsed, accX, accY);
    x = (int)accX;
    accX -= x;
    y = (int)accY;
    accY -= y;
  }else{
    if(valX > maxVal){
      maxVal = valX;
    }
    if(valY > maxVal){
      maxVal = valY;
    }
    if(valX < minVal){
      minVal = valX;
    }
    if(valY < minVal){
      minVal = valY;
    }
  }
}

void MickeysAxis::startCalibration()
{
  calibrating = true;
  prevMaxVal = maxVal;
  maxVal = 0.0f;
  minVal = 0.0f;
}

void MickeysAxis::finishCalibration()
{
  calibrating = false;
  //devise a reasonable profile...
  minVal = fabsf(minVal);
  maxVal = fabsf(maxVal);
  //get lower of those values, so we have full
  //  range in both directions (limit the bigger).
  maxVal = (minVal > maxVal)? maxVal: minVal;
}

void MickeysAxis::cancelCalibration()
{
  calibrating = false;
  maxVal = prevMaxVal;
}

float MickeysAxis::getSpeed(int sens)
{
  float slewTime = timeSlow + (timeFast-timeSlow) * (sens / 100.0);
  return screenMax / slewTime;
}

Mickey::Mickey(QWidget *parent) : QWidget(parent), lbtnSwitch(Qt::Key_F11), 
  updateTimer(this), testTimer(this), /*x("X"), y("Y"),*/
  btnThread(this), state(STANDBY), cdg(this), calDlg(cdg.window()), 
  adg(this), aplDlg(adg.window()), recenterFlag(true)
{
  ui.setupUi(this);
  ui.DZSlider->setValue(m.getDeadZone());
  ui.SensSlider->setValue(m.getSensitivity());
  onOffSwitch = new shortcut(Qt::Key_F9);
  QObject::connect(onOffSwitch, SIGNAL(activated()), this, SLOT(onOffSwitch_activated()));
  QObject::connect(&lbtnSwitch, SIGNAL(activated()), &btnThread, SLOT(on_key_pressed()));
  QObject::connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateTimer_activated()));
  QObject::connect(&btnThread, SIGNAL(clicked()), this, SLOT(threadClicked()));
  QObject::connect(&calDlg, SIGNAL(nextClicked()), this, SLOT(threadClicked()));
  QObject::connect(&calDlg, SIGNAL(cancelClicked()), this, SLOT(calibrationCancelled()));
  QObject::connect(&aplDlg, SIGNAL(keep()), this, SLOT(keepSettings()));
  QObject::connect(&aplDlg, SIGNAL(revert()), this, SLOT(revertSettings()));
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
}

void Mickey::on_CalibrateButton_pressed()
{
  changeState(CALIBRATING);
}

void Mickey::on_ApplyButton_pressed()
{
  std::cout<<"Apply button pressed!"<<std::endl;
  aplDlg.trySettings();
}

void Mickey::pause()
{
  btnThread.setFinish();
  btnThread.wait();
  updateTimer.stop();
  ltr_suspend();
}

void Mickey::wakeup()
{
  updateTimer.start();
  btnThread.start();
  ltr_wakeup();
  ltr_recenter();
}

void Mickey::startCalibration()
{
  cdg.show();
  cdg.raise();
  cdg.activateWindow();
  calDlg.setText((char *)"Center your view and press a button...");
  calState = CENTER;
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
          cdg.hide();
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
      ui.StatusLabel->setText("Tracking");
      break;
    case CALIBRATING:
      ui.StatusLabel->setText("Calibrating");
      break;
    case STANDBY:
      ui.StatusLabel->setText("Paused");
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
      ui.XLabel->setText(QString("X: %1").arg(heading));
      ui.YLabel->setText(QString("Y: %1").arg(pitch));
    }
  }
  int elapsed = updateElapsed.elapsed();
  updateElapsed.restart();
  //reversing signs to get the cursor move according to the head movement
  int dx, dy;
  m.updateAxis(-heading_p, -pitch_p, elapsed, dx, dy);
  if(state == TRACKING){
    uinput.mouseMove(dx, dy);
  }
}

void Mickey::on_DZSlider_valueChanged(int value)
{
  m.changeDeadZone(value);
}

void Mickey::on_SensSlider_valueChanged(int value)
{
  m.changeSensitivity(value);
}

void Mickey::threadClicked()
{
  std::cout<<"thread clicked!!!"<<state<<":"<<calState<<std::endl;
  if(state == CALIBRATING){
    switch(calState){
      case CENTER:
        std::cout<<"Centering..."<<std::endl;
        ltr_recenter();
        calState = CALIBRATE;
        m.startCalibration();
        calDlg.setText((char *)"Move your head to all extremes...");
        break;
      case CALIBRATE:
        std::cout<<"Calibration finished..."<<std::endl;
        m.finishCalibration();
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

void Mickey::calibrationCancelled()
{
  if(state == CALIBRATING){
    m.cancelCalibration();
    cdg.hide();
    calState = CENTER;
    changeState(TRACKING);
  }
}

void Mickey::keepSettings()
{
  std::cout<<"Keeping settings!"<<std::endl;
}

void Mickey::revertSettings()
{
  std::cout<<"Reverting settings!"<<std::endl;
}

