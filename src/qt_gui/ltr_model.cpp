#include "ltr_model.h"
#include "help_view.h"
#include "ltr_gui_prefs.h"
#include "guardian.h"
#include <iostream>
#include <QRegExpValidator>

ModelCreate::ModelCreate(QWidget *parent) : QWidget(parent), validator(NULL)
{
  ui.setupUi(this);
  ui.Model3PtCap->click();
  validator = new QRegExpValidator(QRegExp("^[^\\[\\]]*$"), this);
  ui.ModelName->setValidator(validator);
}

ModelCreate::~ModelCreate()
{
  delete validator;
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
      PREF.addKeyVal(sec, (char *)"Cap-Y", QString::number(ui.CapA->value()));
      PREF.addKeyVal(sec, (char *)"Cap-X", QString::number(ui.CapB->value()));
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
    }else if(ui.ModelFace->isChecked()){
      PREF.addKeyVal(sec, (char *)"Model-type", (char *)"Face");
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

void ModelCreate::on_ModelFace_pressed()
{
  ui.ModelStack->setCurrentIndex(3);
  ui.ModelDescStack->setCurrentIndex(3);
}


ModelEdit::ModelEdit(Guardian *grd, QWidget *parent) : QWidget(parent), modelEditor(0), initializing(false)
{
  grd->regTgt(this);
  ui.setupUi(this);
  mcw = new ModelCreate();
  QObject::connect(mcw, SIGNAL(ModelCreated(const QString &)),
    this, SLOT(ModelCreated(const QString &)));
  refresh();
}

void ModelEdit::refresh()
{
  QString str;
  initializing = true;

  if(PREF.getActiveModel(str)){
    currentSection = str;
    ModelCreated(str);
    on_ModelSelector_activated(str);
  }else{
    ModelCreated("");
  }
  initializing = false;
}

void ModelEdit::on_CreateModelButton_pressed()
{
  mcw->show();
}

void ModelEdit::ModelCreated(const QString &section)
{
  QStringList list;
  ui.ModelSelector->clear();
  if(PREF.getModelList(list)){
    ui.ModelSelector->addItems(list);
  }
  int i = list.indexOf(currentSection);
  if(i != -1){
    ui.ModelSelector->setCurrentIndex(i);
  }else{
    i = list.indexOf(section);
    if(i != -1){
      ui.ModelSelector->setCurrentIndex(i);
      on_ModelSelector_activated(section);
    }else{
      ui.ModelSelector->setCurrentIndex(-1);
    }
  }
}


ModelEdit::~ModelEdit()
{
  if(mcw != NULL){
    delete mcw;
  }
  if(modelEditor != NULL){
    ui.ModelEditorSite->removeWidget(modelEditor);
    delete modelEditor;
    modelEditor = NULL;
  }
}

CapEdit::CapEdit(const QString &section, QWidget *parent) : QWidget(parent), currentSection(section),
  initializing(true)
{
  ui.setupUi(this);
  QString val;
  if(PREF.getKeyVal(currentSection, "Cap-X", val))
    ui.CapB->setValue(val.toFloat());
  if(PREF.getKeyVal(currentSection, "Cap-Y", val))
    ui.CapA->setValue(val.toFloat());
  if(PREF.getKeyVal(currentSection, "Cap-Z", val))
    ui.CapC->setValue(val.toFloat());
  if(PREF.getKeyVal(currentSection, "Head-Y", val))
    ui.CapHy->setValue(val.toFloat());
  if(PREF.getKeyVal(currentSection, "Head-Z", val))
    ui.CapHz->setValue(val.toFloat());
  if(PREF.getKeyVal(currentSection, "Active", val)){
    ui.CapLeds->setCheckState(
      (val.compare("yes", Qt::CaseInsensitive) == 0) ? Qt::Checked : Qt::Unchecked);
  }else{
    ui.CapLeds->setCheckState(Qt::Unchecked);
  }
  initializing = false;
}

CapEdit::~CapEdit()
{
}

void CapEdit::on_CapA_valueChanged(double val)
{ 
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Cap-Y", val);
  PREF.announceModelChange();
}

void CapEdit::on_CapB_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Cap-X", val);
  PREF.announceModelChange();
}

void CapEdit::on_CapC_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Cap-Z", val);
  PREF.announceModelChange();
}

void CapEdit::on_CapHy_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Head-Y", val);
  PREF.announceModelChange();
}

void CapEdit::on_CapHz_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Head-Z", val);
  PREF.announceModelChange();
}

void CapEdit::on_CapLeds_stateChanged(int state)
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



void ModelEdit::on_ModelSelector_activated(const QString &text)
{
  currentSection = text;
  QString type;
  if(!PREF.getKeyVal(currentSection, (char *)"Model-type", type)){
    return;
  }
  QString val;
  if(modelEditor != NULL){
    ui.ModelEditorSite->removeWidget(modelEditor);
    delete modelEditor;
    modelEditor = NULL;
  }
  if(type.compare("Cap", Qt::CaseInsensitive) == 0){
    ui.ModelTypeLabel->setText("3 Point Cap");
    HelpViewer::ChangePage("3ptcap.htm");
    modelEditor = new CapEdit(currentSection, this);
    emit modelSelected(MDL_3PT_CAP);
  }else if(type.compare("Clip", Qt::CaseInsensitive) == 0){
    ui.ModelTypeLabel->setText("3 Point Clip");
    HelpViewer::ChangePage("3ptclip.htm");
    modelEditor = new ClipEdit(currentSection, this);
    emit modelSelected(MDL_3PT_CLIP);
  }else if(type.compare("Face", Qt::CaseInsensitive) == 0){
#warning Add face to model help files!!!
    ui.ModelTypeLabel->setText("Face");
    HelpViewer::ChangePage("1pt.htm");
    emit modelSelected(MDL_FACE);
  }else if(type.compare("SinglePoint", Qt::CaseInsensitive) == 0){
    ui.ModelTypeLabel->setText("1 Point");
    HelpViewer::ChangePage("1pt.htm");
    modelEditor = new SingleEdit(currentSection, this);
    emit modelSelected(MDL_1PT);
  }
  if(modelEditor != NULL){
    ui.ModelEditorSite->addWidget(modelEditor);
  }
  if(!initializing) PREF.activateModel(currentSection);
  PREF.announceModelChange();
}


ClipEdit::ClipEdit(const QString &section, QWidget *parent) : QWidget(parent), currentSection(section),
  initializing(true)
{
  ui.setupUi(this);
  QString val;
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
  
  ui.ClipA->setValue(z1);
  ui.ClipB->setValue(y1);
  ui.ClipC->setValue(y2-y1);
  ui.ClipD->setValue(z1 + z2);
  
  if(PREF.getKeyVal(currentSection, "Head-X", val))
    ui.ClipHx->setValue(val.toFloat());
  if(PREF.getKeyVal(currentSection, "Head-Y", val))
    ui.ClipHy->setValue(val.toFloat());
  if(PREF.getKeyVal(currentSection, "Head-Z", val))
    ui.ClipHz->setValue(val.toFloat());
  if(PREF.getKeyVal(currentSection, "Active", val)){
    ui.ClipLeds->setCheckState(
      (val.compare("yes", Qt::CaseInsensitive) == 0) ? Qt::Checked : Qt::Unchecked);
  }else{
    ui.ClipLeds->setCheckState(Qt::Unchecked);
  }
  initializing = false;
}


ClipEdit::~ClipEdit()
{
}


void ClipEdit::on_ClipA_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Clip-Z1", val);
  PREF.announceModelChange();
}

void ClipEdit::on_ClipB_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Clip-Y1", val);
  PREF.announceModelChange();
}
//                 gui.ClipC->value() + val);

void ClipEdit::on_ClipC_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Clip-Y2", 
                  ui.ClipB->value() + val);
  PREF.announceModelChange();
}

void ClipEdit::on_ClipD_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Clip-Z2", 
       val - ui.ClipA->value());
  PREF.announceModelChange();
}

void ClipEdit::on_ClipHx_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Head-X", val);
  PREF.announceModelChange();
}

void ClipEdit::on_ClipHy_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Head-Y", val);
  PREF.announceModelChange();
}

void ClipEdit::on_ClipHz_valueChanged(double val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Head-Z", val);
  PREF.announceModelChange();
}

void ClipEdit::on_ClipLeds_stateChanged(int state)
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


SingleEdit::SingleEdit(const QString &section, QWidget *parent) : QWidget(parent), currentSection(section),
  initializing(true)
{
  ui.setupUi(this);
  QString val;
  if(PREF.getKeyVal(currentSection, "Active", val)){
    ui.SinglePtLeds->setCheckState(
      (val.compare("yes", Qt::CaseInsensitive) == 0) ? Qt::Checked : Qt::Unchecked);
  }else{
    ui.SinglePtLeds->setCheckState(Qt::Unchecked);
  }
  initializing = false;
}

SingleEdit::~SingleEdit()
{
}


void SingleEdit::on_SinglePtLeds_stateChanged(int state)
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


