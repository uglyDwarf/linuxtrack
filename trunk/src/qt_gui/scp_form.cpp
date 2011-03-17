#include "scp_form.h"
#include <iostream>
#include "ltr_axis.h"
#include "ltr_profiles.h"

ScpForm::ScpForm(QWidget *parent) :QWidget(parent)
{
  ui.setupUi(this);
  yaw = new SCurve(PROFILE.getCurrentProfile()->getYawAxis(), 
                    "Yaw - looking left/right", "Left", "Right", ui.SCPYaw);
  pitch = new SCurve(PROFILE.getCurrentProfile()->getPitchAxis(), 
                      "Pitch - looking up/down", "Down", "Up", ui.SCPPitch);
  roll = new SCurve(PROFILE.getCurrentProfile()->getRollAxis(), 
                     "Roll - tilting head left/right", 
		    "Counter-clockwise", "Clockwise", ui.SCPRoll);
  x = new SCurve(PROFILE.getCurrentProfile()->getTxAxis(), 
                  "Sideways translation", "Left", "Right", ui.SCPX);
  y = new SCurve(PROFILE.getCurrentProfile()->getTyAxis(), 
                  "Up/down translation", "Down", "Up", ui.SCPY);
  z = new SCurve(PROFILE.getCurrentProfile()->getTzAxis(), 
                  "Back/forth translation", "Forth", "Back", ui.SCPZ);
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

