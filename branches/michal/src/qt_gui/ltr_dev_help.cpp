#include "pref_int.h"
#include "ltr_dev_help.h"

LtrDevHelp::LtrDevHelp(ScpForm *s, QWidget *parent) : QWidget(parent), scp(s)
{
  ui.setupUi(this);
}

void LtrDevHelp::on_DumpPrefsButton_pressed()
{
  dump_prefs(NULL);
}

void LtrDevHelp::on_testPitch_valueChanged(int value)
{
  scp->updatePitch(value);
}

void LtrDevHelp::on_testRoll_valueChanged(int value)
{
  scp->updateRoll(value);
}

void LtrDevHelp::on_testYaw_valueChanged(int value)
{
  scp->updateYaw(value);
}

void LtrDevHelp::on_testX_valueChanged(int value)
{
  scp->updateX(value);
}

void LtrDevHelp::on_testY_valueChanged(int value)
{
  scp->updateY(value);
}

void LtrDevHelp::on_testZ_valueChanged(int value)
{
  scp->updateZ(value);
}

