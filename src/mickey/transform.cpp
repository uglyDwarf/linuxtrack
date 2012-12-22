#include "transform.h"
#include <cmath>
#include <iostream>

const int screenMax = 1024;
const float timeFast = 0.1;
const float timeSlow = 4;


MickeysAxis::MickeysAxis(QBoxLayout *parent) : sensitivity(0), deadZone(0), stepOnly(false),
  settings("linuxtrack", "mickey")
{
  ui.setupUi(this);
  parent->addWidget(this);
  parent->addStretch();
  settings.beginGroup("Axes");
  deadZone = settings.value(QString("DeadZone"), 20).toInt();
  sensitivity = settings.value(QString("Sensitivity"), 50).toInt();
  curv = settings.value(QString("Curvature"), 50).toInt();
  stepOnly = settings.value(QString("StepOnly"), false).toBool();
  ui.SensSlider->setValue(sensitivity);
  ui.DZSlider->setValue(deadZone);
  ui.CurveSlider->setValue(curv);
  ui.StepOnly->setCheckState(stepOnly ? Qt::Checked : Qt::Unchecked);
  std::cout<<"DZ: "<<deadZone<<std::endl;
  std::cout<<"Sensitivity: "<<sensitivity<<std::endl;
  std::cout<<"Curvature: "<<curv<<std::endl;
  std::cout<<"StepOnly: "<<(stepOnly ? "true" : "false") <<std::endl;
  settings.endGroup();
}

MickeysAxis::~MickeysAxis()
{
  settings.beginGroup("Axes");
  settings.setValue(QString("DeadZone"), deadZone);
  settings.setValue(QString("Sensitivity"), sensitivity);
  settings.setValue(QString("Curvature"), curv);
  settings.setValue(QString("StepOnly"), stepOnly);
  settings.endGroup();
}

float MickeysAxis::getSpeed(int sens)
{
  float slewTime = timeSlow + (timeFast-timeSlow) * (sens / 100.0);
  return screenMax / slewTime;
}

void MickeysAxis::step(float valX, float valY, int elapsed, float &accX, float &accY)
{
  float mag = sqrtf(valX * valX + valY * valY);
  float angle = atan2f(valY, valX);
  if(mag > 1) mag = 1;
  //deadzone 0 - 50% of the maxValue
  float dz = 0.5 * ((float)deadZone) / 99.0f;
  if(mag < dz){
    mag = 0;
  }else{
    //here can be curve or whatever...
    if(stepOnly){
      mag = 1;
    }else{
      if(curv < 50){
        float c = 1.0 + ((50.0 - curv) / 50.0) * 3.0; //c = (1:4);
        mag = expf(logf(mag) / c);
      }else{
        float c = 1.0 + ((curv - 50.0) / 50.0) * 3.0; //c = (1:4);
        mag = expf(logf(mag) * c);
      }
    }
  }
  accX += mag * cosf(angle) * getSpeed(sensitivity) * (elapsed / 1000.0);
  accY += mag * sinf(angle) * getSpeed(sensitivity) * (elapsed / 1000.0);
}

void MickeysAxis::changeDeadZone(int dz)
{
  deadZone = dz;
}

void MickeysAxis::changeSensitivity(int sens)
{
  sensitivity = sens;
}






MickeyTransform::MickeyTransform(QBoxLayout *parent) : accX(0.0), accY(0.0),
  settings("linuxtrack", "mickey"), calibrating(false), axis(parent)
{
  settings.beginGroup("Transform");
  maxVal = settings.value(QString("Range"), 130).toFloat();
  std::cout<<"Range: "<<maxVal<<std::endl;
  settings.endGroup();
}

MickeyTransform::~MickeyTransform()
{
  settings.beginGroup("Transform");
  settings.setValue(QString("Range"), maxVal);
  settings.endGroup();
}

static float norm(float val)
{
  if(val < -1.0f) return -1.0f;
  if(val > 1.0f) return 1.0f;
  return val;
}

void MickeyTransform::update(float valX, float valY, int elapsed, int &x, int &y)
{
  if(!calibrating){
    axis.step(norm(-valX/maxVal), norm(-valY/maxVal), elapsed, accX, accY);
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

void MickeyTransform::startCalibration()
{
  calibrating = true;
  prevMaxVal = maxVal;
  maxVal = 0.0f;
  minVal = 0.0f;
}

void MickeyTransform::finishCalibration()
{
  calibrating = false;
  //devise a reasonable profile...
  minVal = fabsf(minVal);
  maxVal = fabsf(maxVal);
  //get lower of those values, so we have full
  //  range in both directions (limit the bigger).
  maxVal = (minVal > maxVal)? maxVal: minVal;
}

void MickeyTransform::cancelCalibration()
{
  calibrating = false;
  maxVal = prevMaxVal;
}






/*


//Deadzone - 0 - 50%...
//Sensitivity - normalize input, apply curve, return (-1, 1); outside - convert to speed...
//Dependency on updte rate...



static float sign(float val)
{
  if(val >= 0) return 1.0;
  return -1.0;
}


*/