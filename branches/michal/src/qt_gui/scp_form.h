#ifndef SCP_FORM__H
#define SCP_FORM__H

#include <QWidget>
#include "ui_scp_form.h"
#include "scurve.h"

class ScpForm : public QWidget
{
  Q_OBJECT
 public:
  ScpForm(QWidget *parent = 0);
  ~ScpForm();
  void updatePitch(int newPitch);
  void updateRoll(int newRoll);
  void updateYaw(int newYaw);
  void updateX(int newX);
  void updateY(int newY);
  void updateZ(int newZ);
 private slots:
  void on_SCPCloseButton_pressed();
  
 private:
  Ui::SCPForm ui;
  SCurve *yaw, *pitch, *roll;
  SCurve *x, *y, *z;
};


#endif