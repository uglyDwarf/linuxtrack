#include "pref_int.h"
#include "ltr_dev_help.h"

LtrDevHelp::LtrDevHelp(QWidget *parent) : QWidget(parent)
{
  ui.setupUi(this);
}

void LtrDevHelp::on_DumpPrefsButton_pressed()
{
  dump_prefs(NULL);
}
