#ifndef DEVICE_SETUP__H
#define DEVICE_SETUP__H

#include <QWidget>
#include <ui_device_setup.h>


class DeviceSetup : public QWidget
{
  Q_OBJECT
 public:
  DeviceSetup(QWidget *parent = 0);
  ~DeviceSetup();
  void refresh();
 private:
  Ui::DeviceSetupForm ui;
  static QString descs[8];
  static int orientValues[8];
  QWidget *devPrefs;
  bool initialized;
  void initOrientations();
 private slots:
  void on_DeviceSelector_activated(int index);
  void on_CameraOrientation_activated(int index);
  void on_RefreshDevices_pressed();
};

#endif

