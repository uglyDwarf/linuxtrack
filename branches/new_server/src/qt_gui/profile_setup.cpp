
#include "profile_setup.h"
#include "scp_form.h"


ProfileSetup::ProfileSetup(QWidget *parent) : QWidget(parent), sc(NULL)
{
  ui.setupUi(this);
  sc = new ScpForm();
}


ProfileSetup::~ProfileSetup()
{
  delete sc;
}

void ProfileSetup::on_DetailedAxisSetup_pressed()
{
  sc->show();
}

