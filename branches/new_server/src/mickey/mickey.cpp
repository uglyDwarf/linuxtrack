
#include <QxtGlobalShortcut>
#include <QTimer>
#include <QMutex>
#include "mickey.h"
#include "uinput_ifc.h"
#include "linuxtrack.h"
#include "sn4_com.h"
#include "piper.h"
#include "math_utils.h"
#include <iostream>

QMutex MickeyUinput::mutex;



MickeyCalibration::MickeyCalibration(QWidget *parent): QWidget(parent)
{
  ui.setupUi(this);
}


MickeyUinput::MickeyUinput() : fd(-1)
{
  fd = open_uinput();
  create_device(fd);
}

MickeyUinput::~MickeyUinput()
{
  close_uinput(fd);
  fd = -1;
}

void MickeyUinput::mouseClick(int btns)
{
  mutex.lock();
  click(fd, btns);
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
  lbtnSwitch(Qt::Key_F11, this), fakeBtn(0)
{
  QObject::connect(&lbtnSwitch, SIGNAL(activated()), this, SLOT(on_key_pressed()));
}

//emulate mouse button press using keyboard
void MickeyThread::on_key_pressed()
{
  static int cal_off = 0;
  fakeBtn ^= 1;
  state_t state = parent.getState();
  if(state == TRACKING){
    uinput.mouseClick(fakeBtn);
  }else if(state == CALIBRATING){
    //FSM to detect click (press followed by release)
    switch(cal_off){
      case 0:
        if(fakeBtn != 0){
          cal_off = 1;
        }
        break;
      case 1:
        if(fakeBtn == 0){
          cal_off = 0;
          emit clicked();
        }
        break;
      default:
        cal_off = 0;
        break;
    }
  }
}



void MickeyThread::run()
{
  sn4_btn_event_t ev;
  ssize_t read;
  int cal_off = 0;
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
      if(finish){
        return;
      }
      if(read == sizeof(ev)){
        ev.btns ^= 3;
        state_t state = parent.getState();
        if(state == TRACKING){
          uinput.mouseClick((int)(ev.btns));
        }else if(state == CALIBRATING){
          //FSM to detect click (press followed by release)
          switch(cal_off){
            case 0:
              if(ev.btns != 0){
                cal_off = 1;
              }
              break;
            case 1:
              if(ev.btns == 0){
                cal_off = 0;
                emit clicked();
                //send info of click!
              }
              break;
            default:
              cal_off = 0;
              break;
          }
        }
      }
    }
  }
}
  
  
//Deadzone - 0 - 50%...
//Sensitivity - normalize input, apply curve, return (-1, 1); outside - convert to speed...
//Dependency on updte rate...


MickeysAxis::MickeysAxis(const QString &id) : deadZone(0), sensitivity(0),
  accumulator(0.0), identificator(id), settings("ltr", "mickey"), calibrating(false)
{
  settings.beginGroup("Axes");
  deadZone = settings.value(QString("DeadZone") + identificator, 30).toInt();
  sensitivity = settings.value(QString("Sensitivity") + identificator, 0).toInt();
  maxVal = settings.value(QString("Range") + identificator, 0).toFloat();
  settings.endGroup();
}

MickeysAxis::~MickeysAxis()
{
  settings.beginGroup("Axes");
  settings.setValue(QString("DeadZone") + identificator, deadZone);
  settings.setValue(QString("Sensitivity") + identificator, sensitivity);
  settings.setValue(QString("Range") + identificator, maxVal);
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


float MickeysAxis::processValue(float val)
{
  val /= maxVal; //normalize the value
  if(val > 1) val = 1;
  if(val < 1) val = -1;
  //deadzone 0 - 50% of the maxValue
  float dz = 0.5 * ((float)deadZone) / 99.0f;
  if(val > dz){
    val -= dz;
  }else if(val < -dz){
    val += dz;
  }else{
    val = 0.0;
  }
  val /= (1.0 - dz); //normalize after applying deadzone
  val *= val; //curve attempt...
  val *= sensitivity;
  return val;
}

int MickeysAxis::updateAxis(float val)
{
  if(!calibrating){
    accumulator += processValue(val);
    int res = (int)accumulator;
    accumulator -= res;
    return res;
  }else{
    if(val > maxVal){
      maxVal = val;
    }
    if(val < minVal){
      minVal = val;
    }
    return 0;
  }
}

void MickeysAxis::startCalibration()
{
  calibrating = true;
  maxVal = -1e6;
  minVal = 1e6;
}

void MickeysAxis::finishCalibration()
{
  calibrating = false;
  //devise a reasonable profile...
  minVal *= -1;
  //get lower of those values, so we have full
  //  range in both directions (limit the bigger).
  maxVal = (minVal > maxVal)? maxVal: minVal;
}


Mickey::Mickey(QWidget *parent) : QWidget(parent), updateTimer(this), testTimer(this), 
  x("X"), y("Y"), btnThread(this), state(STANDBY), cdg(this), calDlg(cdg.window())
{
  ui.setupUi(this);
  ui.DZSlider->setValue(x.getDeadZone());
  ui.SensSlider->setValue(x.getSensitivity());
  onOffSwitch = new QxtGlobalShortcut(Qt::Key_F9, this);
  QObject::connect(onOffSwitch, SIGNAL(activated()), this, SLOT(on_onOffSwitch_activated()));
  QObject::connect(&updateTimer, SIGNAL(timeout()), this, SLOT(on_updateTimer_activated()));
  QObject::connect(&btnThread, SIGNAL(clicked()), this, SLOT(on_thread_clicked()));
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

void Mickey::on_onOffSwitch_activated()
{
  switch(state){
    case TRACKING:
      changeState(STANDBY);
      break;
    case STANDBY:
      changeState(TRACKING);
      break;
    default:
      break;
  }
}


void Mickey::on_updateTimer_activated()
{
  static float heading_p = 0.0;
  static float pitch_p = 0.0;
  float heading, pitch, roll, tx, ty, tz;
  unsigned int counter;
  static unsigned int last_counter = 0;
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
  uinput.mouseMove(x.updateAxis(heading_p),y.updateAxis(pitch_p));
}

void Mickey::on_DZSlider_valueChanged(int value)
{
  x.changeDeadZone(value);
  y.changeDeadZone(value);
}

void Mickey::on_SensSlider_valueChanged(int value)
{
  x.changeSensitivity(value);
  y.changeSensitivity(value);
}

void Mickey::on_thread_clicked()
{
  if(state == CALIBRATING){
    switch(calState){
      case CENTER:
        ltr_recenter();
        calState = CALIBRATE;
        x.startCalibration();
        y.startCalibration();
        break;
      case CALIBRATE:
        x.finishCalibration();
        y.finishCalibration();
        break;
      default:
        calState = CENTER;
        break;
    }
  }else{
    changeState(TRACKING);
  }
}
