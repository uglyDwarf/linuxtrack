#include "transform.h"
#include <cmath>
#include <iostream>
#include <QPainter>

const int screenMax = 1024;
const float timeFast = 0.1;
const float timeSlow = 4;


MickeyCurveShow::MickeyCurveShow(QWidget *parent) : QWidget(parent), img(NULL)
{
  //setBackgroundRole(QPalette::Base);
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  setMinimumSize(100, 100);
  setAutoFillBackground(true);
}

void MickeyCurveShow::updatePixmap(QPointF points[], int pointCount)
{
  float s = ((width() > height()) ? height() : width()) - 1;
  if((img == NULL) || (img->width() != s)){
    if(img != NULL){
      delete img;
    }
    img = new QPixmap(s, s);
  }
  for(int i = 0; i < pointCount; ++i){
    points[i] *= (s - 1);
    points[i].setY(s - 1 - points[i].y());
  }
  img->fill();
  QPainter painter(img);
  painter.setPen(Qt::black);
  painter.drawPolyline(points, pointCount);
  painter.end();
  update();
}

void MickeyCurveShow::paintEvent(QPaintEvent * /* event */)
{
  QPainter painter(this);
  painter.drawPixmap(0,0,*img);
  painter.end();
}



MickeysAxis::MickeysAxis(QBoxLayout *parent) : sensitivity(0), deadZone(0), stepOnly(false),
  settings("linuxtrack", "mickey"), curveShow(NULL)
{
  ui.setupUi(this);
  parent->addWidget(this);
  curveShow = new MickeyCurveShow();
  ui.PrefLayout->addWidget(curveShow);
  QObject::connect(curveShow, SIGNAL(resized()), this, SLOT(curveShow_resized()));
  settings.beginGroup("Axes");
  deadZone = settings.value(QString("DeadZone"), 20).toInt();
  sensitivity = settings.value(QString("Sensitivity"), 50).toInt();
  curv = settings.value(QString("Curvature"), 50).toInt();
  stepOnly = settings.value(QString("StepOnly"), false).toBool();
  ui.SensSlider->setValue(sensitivity);
  ui.DZSlider->setValue(deadZone);
  ui.CurveSlider->setValue(curv);
  ui.StepOnly->setCheckState(stepOnly ? Qt::Checked : Qt::Unchecked);
  if(stepOnly){
    ui.CurveSlider->setDisabled(true);
  }else{
    ui.CurveSlider->setEnabled(true);
  }
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

float MickeysAxis::response(float mag)
{
  //deadzone 0 - 50% of the maxValue
  float dz = 0.5 * ((float)deadZone) / 99.0f;
  if(mag <= dz){
    mag = 0;
  }else{
    //here can be curve or whatever...
    if(stepOnly){
      mag = 1;
    }else{
      mag = (mag - dz) / (1.0 - dz);
      if(curv < 50){
        float c = 1.0 + ((50.0 - curv) / 50.0) * 3.0; //c = (1:4);
        mag = expf(logf(mag) / c);
      }else{
        float c = 1.0 + ((curv - 50.0) / 50.0) * 3.0; //c = (1:4);
        mag = expf(logf(mag) * c);
      }
    }
  }
  return mag;
}

void MickeysAxis::step(float valX, float valY, int elapsed, float &accX, float &accY)
{
  float mag = sqrtf(valX * valX + valY * valY);
  float angle = atan2f(valY, valX);
  if(mag > 1) mag = 1;
  
  mag = response(mag);
  
  accX += mag * cosf(angle) * getSpeed(sensitivity) * (elapsed / 1000.0);
  accY += mag * sinf(angle) * getSpeed(sensitivity) * (elapsed / 1000.0);
}

void MickeysAxis::changeDeadZone(int dz)
{
  deadZone = dz;
  updatePixmap();
}

void MickeysAxis::changeSensitivity(int sens)
{
  sensitivity = sens;
}

void MickeysAxis::updatePixmap()
{
  const int pointCount = 128;
  QPointF points[pointCount];
  float x;
  for(int i = 0; i < pointCount; ++i){
    x = (i / (float)(pointCount-1));
    points[i] = QPointF(x, response(x));
  }
  if(curveShow != NULL){
    curveShow->updatePixmap(points, pointCount);
  }
}

void MickeysAxis::on_StepOnly_stateChanged(int state)
{
  stepOnly = (state == Qt::Checked) ? true : false;
  if(stepOnly){
    ui.CurveSlider->setDisabled(true);
  }else{
    ui.CurveSlider->setEnabled(true);
  }
  updatePixmap();
}



MickeyTransform::MickeyTransform(QBoxLayout *parent) : accX(0.0), accY(0.0),
  settings("linuxtrack", "mickey"), calibrating(false), axis(parent)
{
  settings.beginGroup("Transform");
  maxValX = settings.value(QString("RangeX"), 130).toFloat();
  maxValY = settings.value(QString("RangeY"), 130).toFloat();
  std::cout<<"Range: "<<maxValX<<", "<<maxValY<<std::endl;
  settings.endGroup();
}

MickeyTransform::~MickeyTransform()
{
  settings.beginGroup("Transform");
  settings.setValue(QString("RangeX"), maxValX);
  settings.setValue(QString("RangeY"), maxValY);
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
    axis.step(norm(-valX/maxValX), norm(-valY/maxValY), elapsed, accX, accY);
    x = (int)accX;
    accX -= x;
    y = (int)accY;
    accY -= y;
  }else{
    if(valX > maxValX){
      maxValX = valX;
    }
    if(valY > maxValY){
      maxValY = valY;
    }
    if(valX < minValX){
      minValX = valX;
    }
    if(valY < minValY){
      minValY = valY;
    }
  }
}

void MickeyTransform::startCalibration()
{
  calibrating = true;
  prevMaxValX = maxValX;
  maxValX = 0.0f;
  minValX = 0.0f;
  prevMaxValY = maxValY;
  maxValY = 0.0f;
  minValY = 0.0f;
}

void MickeyTransform::finishCalibration()
{
  calibrating = false;
  //devise a reasonable profile...
  minValX = fabsf(minValX);
  maxValX = fabsf(maxValX);
  minValY = fabsf(minValY);
  maxValY = fabsf(maxValY);
  //get lower of those values, so we have full
  //  range in both directions (limit the bigger).
  maxValX = (minValX > maxValX)? maxValX: minValX;
  maxValY = (minValY > maxValY)? maxValY: minValY;
}

void MickeyTransform::cancelCalibration()
{
  calibrating = false;
  maxValX = prevMaxValX;
  maxValY = prevMaxValY;
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