#include "ltr_model.h"
#include "help_view.h"
#include "ltr_gui_prefs.h"
#include "guardian.h"
#include <iostream>
#include <QRegExpValidator>
#include <QMessageBox>
#include <cmath>

ModelCreate::ModelCreate(QWidget *parent) : QWidget(parent), validator(NULL), modelEditor(NULL)
{
  ui.setupUi(this);
  ui.Model3PtCap->click();
  validator = new QRegExpValidator(QRegExp("^[^\\[\\]]*$"), this);
  ui.ModelName->setValidator(validator);
}

ModelCreate::~ModelCreate()
{
  delete validator;
  removeEditor();
  if(modelEditor != NULL){
    delete modelEditor;
    modelEditor = NULL;
  }
}

void ModelCreate::show()
{
  QWidget::show();
  ui.ModelName->clear();
}


void ModelCreate::on_CancelButton_pressed()
{
  close();
}

void ModelCreate::on_CreateButton_pressed()
{
  QString sec = ui.ModelName->text();
  QStringList sectionList;
  PREF.getSectionList(sectionList);
  if(sectionList.contains(sec, Qt::CaseInsensitive)){
    QMessageBox::warning(NULL, "Linuxtrack",
      "The name is already taken, please change the Model name!", QMessageBox::Ok);
    return;
  }
  if(PREF.createSection(sec)){
    if(ui.ModelFace->isChecked()){
      PREF.addKeyVal(sec, (char *)"Model-type", (char *)"Face");
    }else{
      emit dump(sec);
    }
  }
  close();
  emit ModelCreated(sec);
}

void ModelCreate::removeEditor()
{
  if(modelEditor != NULL){
    ui.MdlLayout->removeWidget(modelEditor);
    delete modelEditor;
    modelEditor = NULL;
  }
}

void ModelCreate::activateEditor(QWidget *editor)
{
  removeEditor();
  modelEditor = editor;
  if(modelEditor != NULL){
    ui.MdlLayout->addWidget(modelEditor);
    QObject::connect(this, SIGNAL(dump(const QString &)), modelEditor, SLOT(dump(const QString &)));
  }
}

void ModelCreate::on_Model3PtCap_pressed()
{
  activateEditor(new CapEdit(this));
}

void ModelCreate::on_Model3PtClip_pressed()
{
  activateEditor(new ClipEdit(this));
}

void ModelCreate::on_Model1Pt_pressed()
{
  activateEditor(new SingleEdit(this));
}

void ModelCreate::on_ModelFace_pressed()
{
  activateEditor(NULL);
}


ModelEdit::ModelEdit(Guardian *grd, QWidget *parent) : QWidget(parent), modelTweaker(0), initializing(false)
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

void ModelEdit::on_ModelSelector_activated(const QString &text)
{
  currentSection = text;
  QString type;
  if(!PREF.getKeyVal(currentSection, (char *)"Model-type", type)){
    return;
  }
  QString val;
  if(modelTweaker != NULL){
    ui.ModelEditorSite->removeWidget(modelTweaker);
    delete modelTweaker;
    modelTweaker = NULL;
  }
  if(type.compare("Cap", Qt::CaseInsensitive) == 0){
    ui.ModelTypeLabel->setText("3 Point Cap");
    HelpViewer::ChangePage("3ptcap.htm");
    modelTweaker = new CapTweaking(currentSection, this);
    emit modelSelected(MDL_3PT_CAP);
  }else if(type.compare("Clip", Qt::CaseInsensitive) == 0){
    ui.ModelTypeLabel->setText("3 Point Clip");
    HelpViewer::ChangePage("3ptclip.htm");
    modelTweaker = new ClipTweaking(currentSection, this);
    emit modelSelected(MDL_3PT_CLIP);
  }else if(type.compare("Face", Qt::CaseInsensitive) == 0){
    ui.ModelTypeLabel->setText("Face");
    HelpViewer::ChangePage("1pt.htm");
    emit modelSelected(MDL_FACE);
  }else if(type.compare("SinglePoint", Qt::CaseInsensitive) == 0){
    ui.ModelTypeLabel->setText("1 Point");
    HelpViewer::ChangePage("1pt.htm");
    emit modelSelected(MDL_1PT);
  }
  if(modelTweaker != NULL){
    ui.ModelEditorSite->insertWidget(2, modelTweaker);
  }
  if(!initializing) PREF.activateModel(currentSection);
  PREF.announceModelChange();
}

ModelEdit::~ModelEdit()
{
  if(mcw != NULL){
    delete mcw;
  }
  if(modelTweaker != NULL){
    ui.ModelEditorSite->removeWidget(modelTweaker);
    delete modelTweaker;
    modelTweaker = NULL;
  }
}

CapEdit::CapEdit(QWidget *parent) : QWidget(parent)
{
  ui.setupUi(this);
}

CapEdit::~CapEdit()
{
}

void CapEdit::dump(const QString &sec)
{
  PREF.addKeyVal(sec, (char *)"Model-type", (char *)"Cap");
  PREF.addKeyVal(sec, (char *)"Cap-Y", QString::number(ui.CapA->value()));
  PREF.addKeyVal(sec, (char *)"Cap-X", QString::number(ui.CapB->value()));
  PREF.addKeyVal(sec, (char *)"Cap-Z", QString::number(ui.CapC->value()));
  PREF.addKeyVal(sec, (char *)"Head-Y", "100");
  PREF.addKeyVal(sec, (char *)"Head-Z", "90");
  QString v;
  if(ui.CapLeds->isChecked()){
    v = "yes";
  }else{
    v = "no";
  }
  PREF.addKeyVal(sec, (char *)"Active", v);
}

ClipEdit::ClipEdit(QWidget *parent) : QWidget(parent)
{
  ui.setupUi(this);
}


ClipEdit::~ClipEdit()
{
}

void ClipEdit::dump(const QString &sec)
{
  PREF.addKeyVal(sec, (char *)"Model-type", (char *)"Clip");
  PREF.addKeyVal(sec, (char *)"Clip-Y1", QString::number(ui.ClipB->value()));
  PREF.addKeyVal(sec, (char *)"Clip-Y2", 
    QString::number(ui.ClipB->value() + ui.ClipC->value()));
  PREF.addKeyVal(sec, (char *)"Clip-Z1", QString::number(ui.ClipA->value()));
  PREF.addKeyVal(sec, (char *)"Clip-Z2", 
    QString::number(ui.ClipD->value() - ui.ClipA->value()));
  if(ui.ClipLeft->isChecked()){
    PREF.addKeyVal(sec, (char *)"Head-X", "-100");
  }else{
    PREF.addKeyVal(sec, (char *)"Head-X", "100");
  }
  PREF.addKeyVal(sec, (char *)"Head-Y", "-100");
  PREF.addKeyVal(sec, (char *)"Head-Z", "50");
  
  QString v;
  if(ui.ClipLeds->isChecked()){
    v = "yes";
  }else{
    v = "no";
  }
  PREF.addKeyVal(sec, (char *)"Active", v);
}


SingleEdit::SingleEdit(QWidget *parent) : QWidget(parent)
{
  ui.setupUi(this);
}

SingleEdit::~SingleEdit()
{
}

void SingleEdit::dump(const QString &sec)
{
  PREF.addKeyVal(sec, (char *)"Model-type", (char *)"SinglePoint");
  QString v;
  if(ui.SinglePtLeds->isChecked()){
    v = "yes";
  }else{
    v = "no";
  }
  PREF.addKeyVal(sec, (char *)"Active", v);
}


CapTweaking::CapTweaking(const QString &section, QWidget *parent) : QWidget(parent), currentSection(section),
  initializing(true)
{
  ui.setupUi(this);
  QString val;
  if(PREF.getKeyVal(currentSection, "Head-Y", val))
    ui.CapHy->setValue(val.toFloat());
  if(PREF.getKeyVal(currentSection, "Head-Z", val))
    ui.CapHz->setValue(val.toFloat());
  initializing = false;
}

CapTweaking::~CapTweaking()
{
}

void CapTweaking::on_CapHy_valueChanged(int val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Head-Y", val);
  PREF.announceModelChange();
}

void CapTweaking::on_CapHz_valueChanged(int val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Head-Z", val);
  PREF.announceModelChange();
}

ClipTweaking::ClipTweaking(const QString &section, QWidget *parent) : QWidget(parent), currentSection(section),
  initializing(true)
{
  ui.setupUi(this);
  QString val;
  if(PREF.getKeyVal(currentSection, "Head-X", val)){
    float fval = val.toFloat();
    ui.ClipHx->setValue(fabsf(fval));
    if(fval < 0){
      ui.ClipLeft->setChecked(true);
    }else{
      ui.ClipRight->setChecked(true);
    }
  }
  if(PREF.getKeyVal(currentSection, "Head-Y", val))
    ui.ClipHy->setValue(val.toFloat());
  if(PREF.getKeyVal(currentSection, "Head-Z", val))
    ui.ClipHz->setValue(val.toFloat());
  initializing = false;
}


ClipTweaking::~ClipTweaking()
{
}

void ClipTweaking::tweakHx()
{
  if(!initializing){
    int sign = (ui.ClipLeft->isChecked() ? -1 : 1);
    PREF.setKeyVal(currentSection, (char *)"Head-X", ui.ClipHx->value() * sign);
  }
  PREF.announceModelChange();
}

void ClipTweaking::on_ClipHx_valueChanged(int val)
{
  (void) val;
  tweakHx();
}

void ClipTweaking::on_ClipHy_valueChanged(int val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Head-Y", val);
  PREF.announceModelChange();
}

void ClipTweaking::on_ClipHz_valueChanged(int val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, (char *)"Head-Z", val);
  PREF.announceModelChange();
}

void ClipTweaking::on_ClipLeft_toggled()
{
  tweakHx();
}

void ClipTweaking::on_ClipRight_toggled()
{
  tweakHx();
}

