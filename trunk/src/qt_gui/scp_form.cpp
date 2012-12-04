#include "scp_form.h"
#include <iostream>
#include "ltr_profiles.h"

ScpForm::ScpForm(QWidget *parent) :QWidget(parent)
{
  ui.setupUi(this);
  pitch = new SCurve(PITCH, "Pitch - looking up/down", "Down", "Up", this);
  yaw = new SCurve(YAW, "Yaw - looking left/right", "Left", "Right", this);
  roll = new SCurve(ROLL, "Roll - tilting head left/right", 
		    "Counter-clockwise", "Clockwise", this);
  x = new SCurve(TX, "Sideways translation", "Left", "Right", this);
  y = new SCurve(TY, "Up/down translation", "Down", "Up", this);
  z = new SCurve(TZ, "Back/forth translation", "Forth", "Back", this);
  ui.SCPPitch->addWidget(pitch);
  ui.SCPYaw->addWidget(yaw);
  ui.SCPRoll->addWidget(roll);
  ui.SCPX->addWidget(x);
  ui.SCPY->addWidget(y);
  ui.SCPZ->addWidget(z);
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

