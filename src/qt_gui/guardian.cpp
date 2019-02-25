#include "guardian.h"
#include "device_setup.h"
#include "prefs_link.h"
#include "ltr_model.h"
#include <cstdio>
#include <QMessageBox>

Guardian::Guardian(QWidget *parent) : parentWidget(parent), mdlType(-1), devType(-1), 
  devDesc(QString::fromUtf8(""))
{
}

void Guardian::regTgt(ModelEdit *me)
{
  QObject::connect(me, SIGNAL(modelSelected(int)), this, SLOT(modelSelected(int)));
}

void Guardian::regTgt(DeviceSetup *ds)
{
  QObject::connect(ds, SIGNAL(deviceTypeChanged(int, const QString &)), 
                   this, SLOT(deviceTypeChanged(int, const QString &)));
}

void Guardian::checkDeviceNModel()
{
  if((devType == WEBCAM_FT) || (devType == MACWEBCAM_FT) || (devType == MACPS3EYE_FT)){
    //face tracker needs face model
    if(mdlType != MDL_FACE){
      QMessageBox::warning(parentWidget, QString::fromUtf8("Linuxtrack"),
                           devDesc + QString::fromUtf8(" requires Face type Model!"), QMessageBox::Ok);
    }
  }else if(devType == JOYSTICK){
    //face tracker needs face model
    if(mdlType != MDL_ABSOLUTE){
      QMessageBox::warning(parentWidget, QString::fromUtf8("Linuxtrack"),
                           devDesc + QString::fromUtf8(" requires Absolute type Model!"), QMessageBox::Ok);
    }
  }else{
    //ordinary tracker needs other than face model
    if(mdlType == MDL_FACE){
      QMessageBox::warning(parentWidget, QString::fromUtf8("Linuxtrack"),
                           devDesc + QString::fromUtf8(" won't work correctly with Face type Model!"), 
                           QMessageBox::Ok);
    }else if(mdlType == MDL_ABSOLUTE){
      QMessageBox::warning(parentWidget, QString::fromUtf8("Linuxtrack"),
                           devDesc + QString::fromUtf8(" won't work correctly with Absolute type Model!"), 
                           QMessageBox::Ok);
    }
  }
}

void Guardian::modelSelected(int modelType)
{
  mdlType = modelType;
  if(devType == -1){
    return;
  }
  checkDeviceNModel();
}
  
void Guardian::deviceTypeChanged(int deviceType, const QString &desc)
{
  devType = deviceType;
  devDesc = desc;
  if(mdlType == -1){
    return;
  }
  checkDeviceNModel();
}
  
#include "moc_guardian.cpp"

