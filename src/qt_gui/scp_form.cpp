#include "scp_form.h"
#include <iostream>
#include "ltr_profiles.h"

ScpForm::ScpForm(QWidget *parent) :QWidget(parent)
{
  ui.setupUi(this);
  pitch = new SCurve(PITCH, QString::fromUtf8("Pitch - looking down/up"), QString::fromUtf8("Down"), 
                     QString::fromUtf8("Up"), this);
  yaw = new SCurve(YAW, QString::fromUtf8("Yaw - looking left/right"), QString::fromUtf8("Right"), 
                   QString::fromUtf8("Left"), this);
  roll = new SCurve(ROLL, QString::fromUtf8("Roll - tilting head left/right"), 
		    QString::fromUtf8("Clockwise"), QString::fromUtf8("Counter-clockwise"), this);
  x = new SCurve(TX, QString::fromUtf8("Sideways translation"), QString::fromUtf8("Left"), 
                 QString::fromUtf8("Right"), this);
  y = new SCurve(TY, QString::fromUtf8("Up/down translation"), QString::fromUtf8("Down"), 
                 QString::fromUtf8("Up"), this);
  z = new SCurve(TZ, QString::fromUtf8("Back/forth translation"), QString::fromUtf8("Forth"), 
                 QString::fromUtf8("Back"), this);
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

#include "moc_scp_form.cpp"

