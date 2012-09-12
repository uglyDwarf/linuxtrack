#include "guardian.h"
#include "device_setup.h"
#include "prefs_link.h"
#include "ltr_model.h"

#include <QMessageBox>

Guardian::Guardian(QWidget *parent) : parentWidget(parent), mdlType(-1), devType(-1), devDesc("")
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
  if((devType == WEBCAM_FT) || (devType == WEBCAM_FT)){
    //face tracker needs face model
    if(mdlType != MDL_FACE){
      QMessageBox::warning(parentWidget, "Linuxtrack",
                           devDesc + " requires Face type Model!", QMessageBox::Ok);
    }
  }else{
    //ordinary tracker needs other than face model
    if(mdlType == MDL_FACE){
      QMessageBox::warning(parentWidget, "Linuxtrack",
                           devDesc + " won't work correctly with Face type Model!", QMessageBox::Ok);
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
  




