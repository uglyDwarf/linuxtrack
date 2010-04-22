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
  void updatePitch(float newPitch);
  void updateRoll(float newRoll);
  void updateYaw(float newYaw);
  void updateX(float newX);
  void updateY(float newY);
  void updateZ(float newZ);
  void setSlaves(QDoubleSpinBox *pitchLM, QDoubleSpinBox *pitchRM,
                 QDoubleSpinBox *rollLM, QDoubleSpinBox *rollRM,
                 QDoubleSpinBox *yawLM, QDoubleSpinBox *yawRM,
                 QDoubleSpinBox *xLM, QDoubleSpinBox *xRM,
                 QDoubleSpinBox *yLM, QDoubleSpinBox *yRM,
                 QDoubleSpinBox *zLM, QDoubleSpinBox *zRM
                 );
 private slots:
  void on_SCPCloseButton_pressed();
  void reinit();
 private:
  Ui::SCPForm ui;
  SCurve *yaw, *pitch, *roll;
  SCurve *x, *y, *z;
};


#endif
