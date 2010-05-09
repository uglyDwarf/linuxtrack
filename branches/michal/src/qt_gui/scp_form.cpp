#include "scp_form.h"
#include <iostream>
#include "ltr_axis.h"

ScpForm::ScpForm(QWidget *parent) :QWidget(parent)
{
  ui.setupUi(this);
  yaw = new SCurve(YAW, "Yaw - looking left/right", "Left", "Right", ui.SCPYaw);
  pitch = new SCurve(PITCH, "Pitch - looking up/down", "Down", "Up", ui.SCPPitch);
  roll = new SCurve(ROLL, "Roll - tilting head left/right", 
		    "Counter-clockwise", "Clockwise", ui.SCPRoll);
  x = new SCurve(XTR, "Sideways translation", "Left", "Right", ui.SCPX);
  y = new SCurve(YTR, "Up/down translation", "Down", "Up", ui.SCPY);
  z = new SCurve(ZTR, "Back/forth translation", "Forth", "Back", ui.SCPZ);
}

void ScpForm::reinit()
{
  yaw->reinit();
  pitch->reinit();
  roll->reinit();
  x->reinit();
  y->reinit();
  z->reinit();
}

void ScpForm::setSlaves(QCheckBox *pitchEn, QDoubleSpinBox *pitchLM, QDoubleSpinBox *pitchRM,
                        QCheckBox *rollEn, QDoubleSpinBox *rollLM, QDoubleSpinBox *rollRM,
                        QCheckBox *yawEn, QDoubleSpinBox *yawLM, QDoubleSpinBox *yawRM,
                        QCheckBox *xEn, QDoubleSpinBox *xLM, QDoubleSpinBox *xRM,
                        QCheckBox *yEn, QDoubleSpinBox *yLM, QDoubleSpinBox *yRM,
                        QCheckBox *zEn, QDoubleSpinBox *zLM, QDoubleSpinBox *zRM
                        )
{
  pitch->setSlaves(pitchEn, pitchLM, pitchRM);
  roll->setSlaves(rollEn, rollLM, rollRM);
  yaw->setSlaves(yawEn, yawLM, yawRM);
  x->setSlaves(xEn, xLM, xRM);
  y->setSlaves(yEn, yLM, yRM);
  z->setSlaves(zEn, zLM, zRM);
}

ScpForm::~ScpForm()
{
  delete yaw;
  delete pitch;
  delete roll;
  delete x;
  delete y;
  delete z;
}

void ScpForm::on_SCPCloseButton_pressed()
{
  close();
}

void ScpForm::updatePitch(float newPitch)
{
  pitch->movePoint(newPitch);
}

void ScpForm::updateRoll(float newRoll)
{
  roll->movePoint(newRoll);
}

void ScpForm::updateYaw(float newYaw)
{
  yaw->movePoint(newYaw);
}

void ScpForm::updateX(float newX)
{
  x->movePoint(newX);
}

void ScpForm::updateY(float newY)
{
  y->movePoint(newY);
}

void ScpForm::updateZ(float newZ)
{
  z->movePoint(newZ);
}

