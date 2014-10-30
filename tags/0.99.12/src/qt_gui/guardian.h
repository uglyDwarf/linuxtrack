#ifndef GUARDIAN__H
#define GUARDIAN__H

#include <QObject>
#include <QString>

class ModelEdit;
class DeviceSetup;

class Guardian : QObject
{
  Q_OBJECT
 public:
  Guardian(QWidget *parent);
  void regTgt(ModelEdit *me);
  void regTgt(DeviceSetup *ds);
 private:
  QWidget *parentWidget;
  int mdlType;
  int devType;
  QString devDesc;
  void checkDeviceNModel();
 private slots:
  void modelSelected(int modelType);
  void deviceTypeChanged(int deviceType, const QString &desc);
};



#endif
