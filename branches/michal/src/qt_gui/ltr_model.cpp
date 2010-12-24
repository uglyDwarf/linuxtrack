#include "ltr_model.h"
#include "ltr_gui_prefs.h"
#include <iostream>

ModelCreate::ModelCreate(QWidget *parent) : QWidget(parent)
{
  ui.setupUi(this);
  ui.Model3PtCap->click();
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
    }else if(ui.Model3PtClip->isChecked()){
      PREF.addKeyVal(sec, (char *)"Model-type", (char *)"Clip");
      PREF.addKeyVal(sec, (char *)"Clip-Y1", QString::number(ui.ClipB->value()));
      PREF.addKeyVal(sec, (char *)"Clip-Y2", 
           QString::number(ui.ClipB->value() + ui.ClipC->value()));
      PREF.addKeyVal(sec, (char *)"Clip-Z1", QString::number(ui.ClipA->value()));
      PREF.addKeyVal(sec, (char *)"Clip-Z2", 
           QString::number(ui.ClipD->value() - ui.ClipA->value()));
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
    }else{//1pt
      PREF.addKeyVal(sec, (char *)"Model-type", (char *)"SinglePoint");
      QString v;
      if(ui.SinglePtLeds->checkState() == Qt::Checked){
        v = "yes";
      }else{
        v = "no";
      }
      PREF.addKeyVal(sec, (char *)"Active", v);
    }
  }
  close();
  emit ModelCreated(sec);
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

void ModelCreate::on_Model1Pt_pressed()
{
  ui.ModelStack->setCurrentIndex(2);
  ui.ModelDescStack->setCurrentIndex(2);
}

ModelEdit::ModelEdit(const Ui::LinuxtrackMainForm &ui) : gui(ui), initializing(false)
{
  mcw = new ModelCreate();
  Connect();
  refresh();
}

void ModelEdit::refresh()
{
  QString str;
  initializing = true;

  if(PREF.getActiveModel(str)){
    currentSection = str;
    on_ModelCreated(str);
    on_ModelSelector_activated(str);
  }else{
    on_ModelCreated("");
    gui.ModelStack->setCurrentIndex(3);
  }
  initializing = false;
}

void ModelEdit::on_CreateModelButton_pressed()
{
  mcw->show();
}

void ModelEdit::on_ModelCreated(const QString &section)
{
  QStringList list;
  gui.ModelSelector->clear();
  if(PREF.getModelList(list)){
    gui.ModelSelector->addItems(list);
  }
  int i = list.indexOf(currentSection);
  if(i != -1){
    gui.ModelSelector->setCurrentIndex(i);
  }else{
    i = list.indexOf(section);
    if(i != -1){
      gui.ModelSelector->setCurrentIndex(i);
      on_ModelSelector_activated(section);
    }else{
      gui.ModelSelector->setCurrentIndex(-1);
    }
  }
}


ModelEdit::~ModelEdit()
{
}

void ModelEdit::Connect()
{
  QObject::connect(gui.CreateModelButton, SIGNAL(pressed()),
    this, SLOT(on_CreateModelButton_pressed()));
  QObject::connect(mcw, SIGNAL(ModelCreated(const QString &)),
    this, SLOT(on_ModelCreated(const QString &)));
  QObject::connect(gui.ModelSelector, SIGNAL(activated(const QString&)),
    this, SLOT(on_ModelSelector_activated(const QString&)));
  
  QObject::connect(gui.CapA, SIGNAL(valueChanged(double)),
    this, SLOT(on_CapA_valueChanged(double)));
  QObject::connect(gui.CapB, SIGNAL(valueChanged(double)),
    this, SLOT(on_CapB_valueChanged(double)));
  QObject::connect(gui.CapC, SIGNAL(valueChanged(double)),
    this, SLOT(on_CapC_valueChanged(double)));
  QObject::connect(gui.CapHy, SIGNAL(valueChanged(double)),
    this, SLOT(on_CapHy_valueChanged(double)));
  QObject::connect(gui.CapHz, SIGNAL(valueChanged(double)),
    this, SLOT(on_CapHz_valueChanged(double)));

  QObject::connect(gui.ClipA, SIGNAL(valueChanged(double)),
    this, SLOT(on_ClipA_valueChanged(double)));
  QObject::connect(gui.ClipB, SIGNAL(valueChanged(double)),
    this, SLOT(on_ClipB_valueChanged(double)));
  QObject::connect(gui.ClipC, SIGNAL(valueChanged(double)),
    this, SLOT(on_ClipC_valueChanged(double)));
  QObject::connect(gui.ClipD, SIGNAL(valueChanged(double)),
    this, SLOT(on_ClipD_valueChanged(double)));
  QObject::connect(gui.ClipHx, SIGNAL(valueChanged(double)),
    this, SLOT(on_ClipHx_valueChanged(double)));
  QObject::connect(gui.ClipHy, SIGNAL(valueChanged(double)),
    this, SLOT(on_ClipHy_valueChanged(double)));
  QObject::connect(gui.ClipHz, SIGNAL(valueChanged(double)),
    this, SLOT(on_ClipHz_valueChanged(double)));
  
  QObject::connect(gui.CapLeds, SIGNAL(stateChanged(int)),
    this, SLOT(on_CapLeds_stateChanged(int)));
  QObject::connect(gui.ClipLeds, SIGNAL(stateChanged(int)),
    this, SLOT(on_ClipLeds_stateChanged(int)));
} 

void ModelEdit::on_ModelSelector_activated(const QString &text)
{
  currentSection = text;
  QString type;
  if(!PREF.getKeyVal(currentSection, (char *)"Model-type", type)){
    return;
  }
  QString val;
  if(type.compare("Cap", Qt::CaseInsensitive) == 0){
    gui.ModelTypeLabel->setText("3 Point Cap");
    gui.ModelStack->setCurrentIndex(0);
    gui.ModelDescStack->setCurrentIndex(0);
    if(PREF.getKeyVal(currentSection, "Cap-X", val))
      gui.CapA->setValue(val.toFloat());
    if(PREF.getKeyVal(currentSection, "Cap-Y", val))
      gui.CapB->setValue(val.toFloat());
    if(PREF.getKeyVal(currentSection, "Cap-Z", val))
      gui.CapC->setValue(val.toFloat());
    if(PREF.getKeyVal(currentSection, "Head-Y", val))
      gui.CapHy->setValue(val.toFloat());
    if(PREF.getKeyVal(currentSection, "Head-Z", val))
      gui.CapHz->setValue(val.toFloat());
    if(PREF.getKeyVal(currentSection, "Active", val)){
      gui.CapLeds->setCheckState(
        (val.compare("yes", Qt::CaseInsensitive) == 0) ? Qt::Checked : Qt::Unchecked);
    }else{
      gui.CapLeds->setCheckState(Qt::Unchecked);
    }
  }else if(type.compare("Clip", Qt::CaseInsensitive) == 0){
    gui.ModelTypeLabel->setText("3 Point Clip");
    gui.ModelStack->setCurrentIndex(1);
    gui.ModelDescStack->setCurrentIndex(1);
    float y1 = 40;
    float y2 = 110;
    float z1 = 30;
    float z2 = 50;
    if(PREF.getKeyVal(currentSection, "Clip-Y1", val))
      y1 = val.toFloat();
    if(PREF.getKeyVal(currentSection, "Clip-Y2", val))
      y2 = val.toFloat();
    if(PREF.getKeyVal(currentSection, "Clip-Z1", val))
      z1 = val.toFloat();
    if(PREF.getKeyVal(currentSection, "Clip-Z2", val))
      z2 = val.toFloat();

    gui.ClipA->setValue(z1);
    gui.ClipB->setValue(y1);
    gui.ClipC->setValue(y2-y1);
    gui.ClipD->setValue(z1 + z2);
    
    if(PREF.getKeyVal(currentSection, "Head-X", val))
      gui.ClipHx->setValue(val.toFloat());
    if(PREF.getKeyVal(currentSection, "Head-Y", val))
      gui.ClipHy->setValue(val.toFloat());
    if(PREF.getKeyVal(currentSection, "Head-Z", val))
      gui.ClipHz->setValue(val.toFloat());
    if(PREF.getKeyVal(currentSection, "Active", val)){
      gui.ClipLeds->setCheckState(
        (val.compare("yes", Qt::CaseInsensitive) == 0) ? Qt::Checked : Qt::Unchecked);
    }else{
      gui.ClipLeds->setCheckState(Qt::Unchecked);
    }
  }else{//1pt
    gui.ModelTypeLabel->setText("1 Point");
    gui.ModelStack->setCurrentIndex(2);
    gui.ModelDescStack->setCurrentIndex(2);
    if(PREF.getKeyVal(currentSection, "Active", val)){
      gui.SinglePtLeds->setCheckState(
        (val.compare("yes", Qt::CaseInsensitive) == 0) ? Qt::Checked : Qt::Unchecked);
    }else{
      gui.SinglePtLeds->setCheckState(Qt::Unchecked);
    }
  }
  if(!initializing) PREF.activateModel(currentSection);
  PREF.announceModelChange();
}

void ModelEdit::on_CapA_valueChanged(double val)
{ 
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Cap-X", val);
  PREF.announceModelChange();
}

void ModelEdit::on_CapB_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Cap-Y", val);
  PREF.announceModelChange();
}

void ModelEdit::on_CapC_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Cap-Z", val);
  PREF.announceModelChange();
}

void ModelEdit::on_CapHy_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Head-Y", val);
  PREF.announceModelChange();
}

void ModelEdit::on_CapHz_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Head-Z", val);
  PREF.announceModelChange();
}

void ModelEdit::on_CapLeds_stateChanged(int state)
{
  QString v;
  if(state == Qt::Checked){
    v = "yes";
  }else{
    v = "no";
  }
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Active", v);
  PREF.announceModelChange();
}

void ModelEdit::on_ClipA_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Clip-Z1", val);
  PREF.announceModelChange();
}

void ModelEdit::on_ClipB_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Clip-Y1", val);
  PREF.announceModelChange();
}
//                 gui.ClipC->value() + val);

void ModelEdit::on_ClipC_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Clip-Y2", 
                  gui.ClipB->value() + val);
  PREF.announceModelChange();
}

void ModelEdit::on_ClipD_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Clip-Z2", 
       val - gui.ClipA->value());
  PREF.announceModelChange();
}

void ModelEdit::on_ClipHx_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Head-X", val);
  PREF.announceModelChange();
}

void ModelEdit::on_ClipHy_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Head-Y", val);
  PREF.announceModelChange();
}

void ModelEdit::on_ClipHz_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Head-Z", val);
  PREF.announceModelChange();
}

void ModelEdit::on_ClipLeds_stateChanged(int state)
{
  QString v;
  if(state == Qt::Checked){
    v = "yes";
  }else{
    v = "no";
  }
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Active", v);
  PREF.announceModelChange();
}

void ModelEdit::on_SinglePtLeds_stateChanged(int state)
{
  QString v;
  if(state == Qt::Checked){
    v = "yes";
  }else{
    v = "no";
  }
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Active", v);
  PREF.announceModelChange();
}

