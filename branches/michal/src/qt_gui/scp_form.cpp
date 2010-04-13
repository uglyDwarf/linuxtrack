#include "scp_form.h"

ScpForm::ScpForm(QWidget *parent) :QWidget(parent)
{
  ui.setupUi(this);
  yaw = new SCurve("Yaw", "Yaw - looking left/right", "Left", "Right", ui.SCPYaw);
  pitch = new SCurve("Pitch", "Pitch - looking up/down", "Up", "Down", ui.SCPPitch);
  roll = new SCurve("Roll", "Roll - tilting head left/right", 
		    "Counter-clockwise", "Clockwise", ui.SCPRoll);
  x = new SCurve("Xtranslation", "Sideways translation", "Left", "Right", ui.SCPX);
  y = new SCurve("Ytranslation", "Up/down translation", "Up", "Down", ui.SCPY);
  z = new SCurve("Ztranslation", "Back/forth translation", "Back", "Forth", ui.SCPZ);
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