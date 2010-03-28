#include "ltr_model.h"
#include "ltr_gui_prefs.h"
#include <iostream>

ModelCreate::ModelCreate(QWidget *parent) : QWidget(parent)
{
  ui.setupUi(this);
}

ModelCreate::~ModelCreate()
{
}

void ModelCreate::on_CancelButton_pressed()
{
  close();
}

void ModelCreate::on_CreateButton_pressed()
{
  QString sec = ui.ModelName->text();
  if(PREF.createSection(sec)){
    if(ui.Model3PtCap->isChecked()){
      PREF.addKeyVal(sec, (char *)"Model-type", (char *)"Cap");
      PREF.addKeyVal(sec, (char *)"Cap-X", QString::number(ui.CapA->value()));
      PREF.addKeyVal(sec, (char *)"Cap-Y", QString::number(ui.CapB->value()));
      PREF.addKeyVal(sec, (char *)"Cap-Z", QString::number(ui.CapC->value()));
      PREF.addKeyVal(sec, (char *)"Head-Y", QString::number(ui.CapHy->value()));
      PREF.addKeyVal(sec, (char *)"Head-Z", QString::number(ui.CapHz->value()));
      QString v;
      if(ui.CapLeds->checkState() == Qt::Checked){
        v = "yes";
      }else{
        v = "no";
      }
      PREF.addKeyVal(sec, (char *)"Active", v);
    }else{
      PREF.addKeyVal(sec, (char *)"Model-type", (char *)"Clip");
      PREF.addKeyVal(sec, (char *)"Clip-Y1", QString::number(ui.ClipA->value()));
      PREF.addKeyVal(sec, (char *)"Clip-Y2", 
           QString::number(ui.ClipA->value() + ui.ClipB->value()));
      PREF.addKeyVal(sec, (char *)"Clip-Z1", QString::number(ui.ClipC->value()));
      PREF.addKeyVal(sec, (char *)"Clip-Z2", QString::number(ui.ClipD->value()));
      PREF.addKeyVal(sec, (char *)"Head-X", QString::number(ui.ClipHx->value()));
      PREF.addKeyVal(sec, (char *)"Head-Y", QString::number(ui.ClipHy->value()));
      PREF.addKeyVal(sec, (char *)"Head-Z", QString::number(ui.ClipHz->value()));
      
      QString v;
      if(ui.ClipLeds->checkState() == Qt::Checked){
        v = "yes";
      }else{
        v = "no";
      }
      PREF.addKeyVal(sec, (char *)"Active", v);
    }
  }
  close();
}

void ModelCreate::on_Model3PtCap_pressed()
{
  ui.ModelStack->setCurrentIndex(0);
  ui.ModelDescStack->setCurrentIndex(0);
}

void ModelCreate::on_Model3PtClip_pressed()
{
  ui.ModelStack->setCurrentIndex(1);
  ui.ModelDescStack->setCurrentIndex(1);
}

