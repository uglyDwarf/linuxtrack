#include "wine_warn.h"


WineWarn::WineWarn(QWidget *parent = 0) : QDialog(parent)
{
  ui.setupUi(this);
}

WineWarn::~WineWarn()
{
}

void WineWarn::show()
{
  QDialog::show();
}

void WineWarn::on_OKButton_pressed()
{
  if(ui.DontShow->isChecked()){
    accept();
  }else{
    reject();
  }
}



