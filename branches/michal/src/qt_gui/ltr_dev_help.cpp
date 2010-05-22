#include "pref_int.h"
#include "ltr_dev_help.h"

LtrDevHelp::LtrDevHelp(QWidget *parent) : QWidget(parent)
{
  ui.setupUi(this);
}

void LtrDevHelp::on_DumpPrefsButton_pressed()
{
  ltr_int_dump_prefs(NULL);
}
