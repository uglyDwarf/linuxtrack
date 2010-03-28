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
      PREF.addKeyVal(sec, (char *)"Clip-Z2", 
           QString::number(ui.ClipD->value() + ui.ClipA->value()));
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
  emit ModelCreated();
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


ModelEdit::ModelEdit(const Ui::LinuxtrackMainForm &ui) : gui(ui)
{
  mcw = new ModelCreate();
  Connect();
  on_ModelCreated();
  QString str;
  if(PREF.getActiveModel(str)){
    currentSection = str;
    on_ModelSelector_activated(str);
  }//else...!!!
}

void ModelEdit::on_CreateModelButton_pressed()
{
  mcw->show();
}

void ModelEdit::on_ModelCreated()
{
  QStringList list;
  gui.ModelSelector->clear();
  if(PREF.getModelList(list)){
    gui.ModelSelector->addItems(list);
  }
  int i = list.indexOf(currentSection);
  if(i != -1){
    gui.ModelSelector->setCurrentIndex(i);
  }
}


ModelEdit::~ModelEdit()
{
}

void ModelEdit::Connect()
{
  QObject::connect(gui.CreateModelButton, SIGNAL(pressed()),
    this, SLOT(on_CreateModelButton_pressed()));
  QObject::connect(mcw, SIGNAL(ModelCreated()),
    this, SLOT(on_ModelCreated()));
  QObject::connect(gui.ModelSelector, SIGNAL(activated(const QString&)),
    this, SLOT(on_ModelSelector_activated(const QString&)));
} 

void ModelEdit::on_ModelSelector_activated(const QString &text)
{
  currentSection = text;
  QString type;
  if(!PREF.getKeyVal(currentSection, (char *)"Model-type", type)){
    return;
  }
  if(type.compare("Cap", Qt::CaseInsensitive) == 0){
    gui.ModelTypeLabel->setText("3 Point Cap");
    gui.ModelStack->setCurrentIndex(0);
    gui.ModelDescStack->setCurrentIndex(0);
  }else if(type.compare("Clip", Qt::CaseInsensitive) == 0){
    gui.ModelTypeLabel->setText("3 Point Clip");
    gui.ModelStack->setCurrentIndex(1);
    gui.ModelDescStack->setCurrentIndex(1);
  }else{//1pt
    gui.ModelTypeLabel->setText("1 Point");
    gui.ModelStack->setCurrentIndex(2);
    gui.ModelDescStack->setCurrentIndex(2);
  }
}

void ModelEdit::on_CapA_valueChanged(double val)
{
  PREF.setKeyVal(currentSection, (char *)"Cap-X", val);
}

void ModelEdit::on_CapB_valueChanged(double val)
{
  PREF.setKeyVal(currentSection, (char *)"Cap-Y", val);
}

void ModelEdit::on_CapC_valueChanged(double val)
{
  PREF.setKeyVal(currentSection, (char *)"Cap-Z", val);
}

void ModelEdit::on_CapHy_valueChanged(double val)
{
  PREF.setKeyVal(currentSection, (char *)"Head-Y", val);
}

void ModelEdit::on_CapHz_valueChanged(double val)
{
  PREF.setKeyVal(currentSection, (char *)"Head-Z", val);
}

void ModelEdit::on_CapLeds_stateChanged(int state)
{
  QString v;
  if(state == Qt::Checked){
    v = "yes";
  }else{
    v = "no";
  }
  PREF.setKeyVal(currentSection, (char *)"Active", v);
}

void ModelEdit::on_ClipA_valueChanged(double val)
{
  PREF.setKeyVal(currentSection, (char *)"Clip-Z1", val);
}

void ModelEdit::on_ClipB_valueChanged(double val)
{
  PREF.setKeyVal(currentSection, (char *)"Clip-Y1", val);
}
//                 gui.ClipC->value() + val);

void ModelEdit::on_ClipC_valueChanged(double val)
{
  PREF.setKeyVal(currentSection, (char *)"Clip-Y2", 
                  gui.ClipB->value() + val);
}

void ModelEdit::on_ClipD_valueChanged(double val)
{
  PREF.setKeyVal(currentSection, (char *)"Clip-Z2", 
       val - gui.ClipA->value());
}

void ModelEdit::on_ClipHx_valueChanged(double val)
{
  PREF.setKeyVal(currentSection, (char *)"Head-X", val);
}

void ModelEdit::on_ClipHy_valueChanged(double val)
{
  PREF.setKeyVal(currentSection, (char *)"Head-Y", val);
}

void ModelEdit::on_ClipHz_valueChanged(double val)
{
  PREF.setKeyVal(currentSection, (char *)"Head-Z", val);
}

void ModelEdit::on_ClipLeds_stateChanged(int state)
{
  QString v;
  if(state == Qt::Checked){
    v = "yes";
  }else{
    v = "no";
  }
  PREF.setKeyVal(currentSection, (char *)"Active", v);
}

