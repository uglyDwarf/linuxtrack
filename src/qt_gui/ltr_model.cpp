#include "ltr_model.h"
#include "help_view.h"
#include "ltr_gui_prefs.h"
#include "guardian.h"
#include <iostream>
#include <QRegExpValidator>
#include <QMessageBox>
#include <cmath>

ModelCreate::ModelCreate(QWidget *parent) : QDialog(parent), validator(NULL), modelEditor(NULL)
{
  ui.setupUi(this);
  validator = new QRegExpValidator(QRegExp(QString::fromUtf8("^[^\\[\\]]*$")), this);
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

int ModelCreate::exec()
{
  ui.ModelName->clear();
  ui.ModelName->setFocus();
  return QDialog::exec();
}


void ModelCreate::on_CancelButton_pressed()
{
  reject();
}

void ModelCreate::on_CreateButton_pressed()
{
  QString sec = ui.ModelName->text();
  QStringList sectionList;
  PREF.getSectionList(sectionList);
  if(sec.isEmpty()){
    QMessageBox::warning(NULL, QString::fromUtf8("Linuxtrack"),
      QString::fromUtf8("Please specify the Model name!"), QMessageBox::Ok);
    ui.ModelName->setFocus();
    return;
  }
  if(sectionList.contains(sec, Qt::CaseInsensitive)){
    QMessageBox::warning(NULL, QString::fromUtf8("Linuxtrack"),
      QString::fromUtf8("The name is already taken, please change the Model name!"), QMessageBox::Ok);
    return;
  }
  if(PREF.createSection(sec)){
    if(ui.ModelTypeCombo->currentIndex() == 3){
      PREF.addKeyVal(sec, QString::fromUtf8("Model-type"), QString::fromUtf8("Face"));
    }else if(ui.ModelTypeCombo->currentIndex() == 4){
      PREF.addKeyVal(sec, QString::fromUtf8("Model-type"), QString::fromUtf8("Absolute"));
    }else{
      emit dump(sec);
    }
  }
  emit ModelCreated(sec);
  accept();
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
  }else{
    
  }
}

void ModelCreate::on_ModelTypeCombo_currentIndexChanged(int index)
{
  switch(index){
    case 0:
      activateEditor(new CapEdit(this));
      break;
    case 1:
      activateEditor(new ClipEdit(this));
      break;
    case 2:
      activateEditor(new SingleEdit(this));
      break;
    default:
      activateEditor(NULL);      
      break;
  }
}


ModelEdit::ModelEdit(Guardian *grd, QWidget *parent) : QWidget(parent), modelTweaker(0), initializing(false)
{
  grd->regTgt(this);
  ui.setupUi(this);
  mcw = new ModelCreate(this);
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
    ModelCreated(QString::fromUtf8(""));
  }
  initializing = false;
}

void ModelEdit::on_CreateModelButton_pressed()
{
  mcw->exec();
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
  modelType_t modelType = MDL_1PT;
  currentSection = text;
  QString type;
  if(!PREF.getKeyVal(currentSection, QString::fromUtf8("Model-type"), type)){
    return;
  }
  QString val;
  if(modelTweaker != NULL){
    ui.ModelEditorSite->removeWidget(modelTweaker);
    delete modelTweaker;
    modelTweaker = NULL;
  }
  if(type.compare(QString::fromUtf8("Cap"), Qt::CaseInsensitive) == 0){
    //ui.ModelTypeLabel->setText("3 Point Cap");
    if(text == QString::fromUtf8("NP TrackClip")){
      ui.ModelPreview->setPixmap(QPixmap(QString::fromUtf8(":/ltr/cap_np.png")));
    }else{
      ui.ModelPreview->setPixmap(QPixmap(QString::fromUtf8(":/ltr/cap_1.png")));
    }
    modelTweaker = new CapTweaking(currentSection, this);
    
    modelType = MDL_3PT_CAP;
  }else if(type.compare(QString::fromUtf8("Clip"), Qt::CaseInsensitive) == 0){
    //ui.ModelTypeLabel->setText("3 Point Clip");
    if(text == QString::fromUtf8("NP TrackClip Pro")){
      ui.ModelPreview->setPixmap(QPixmap(QString::fromUtf8(":/ltr/clip_np.png")));
    }else{
      ui.ModelPreview->setPixmap(QPixmap(QString::fromUtf8(":/ltr/clip_1.png")));
    }
    modelTweaker = new ClipTweaking(currentSection, this);
    modelType = MDL_3PT_CLIP;
  }else if(type.compare(QString::fromUtf8("Face"), Qt::CaseInsensitive) == 0){
    //ui.ModelTypeLabel->setText("Face");
    ui.ModelPreview->setPixmap(QPixmap(QString::fromUtf8(":/ltr/face.png")));
    modelType = MDL_FACE;
    modelTweaker = NULL;
  }else if(type.compare(QString::fromUtf8("Absolute"), Qt::CaseInsensitive) == 0){
    //ui.ModelTypeLabel->setText("Absolute");
    ui.ModelPreview->setPixmap(QPixmap(QString::fromUtf8(":/ltr/face.png")));
    modelType = MDL_ABSOLUTE;
    modelTweaker = NULL;
  }else if(type.compare(QString::fromUtf8("SinglePoint"), Qt::CaseInsensitive) == 0){
    //ui.ModelTypeLabel->setText("1 Point");
    ui.ModelPreview->setPixmap(QPixmap(QString::fromUtf8(":/ltr/single.png")));
    modelType = MDL_1PT;
    modelTweaker = NULL;
  }
  if(modelTweaker != NULL){
    ui.ModelEditorSite->insertWidget(2, modelTweaker);
  }
  if(!initializing) PREF.activateModel(currentSection);
  PREF.announceModelChange();
  emit modelSelected(modelType);
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
  PREF.addKeyVal(sec, QString::fromUtf8("Model-type"), QString::fromUtf8("Cap"));
  PREF.addKeyVal(sec, QString::fromUtf8("Cap-Y"), QString::number(ui.CapA->value()));
  PREF.addKeyVal(sec, QString::fromUtf8("Cap-X"), QString::number(ui.CapB->value()));
  PREF.addKeyVal(sec, QString::fromUtf8("Cap-Z"), QString::number(ui.CapC->value()));
  PREF.addKeyVal(sec, QString::fromUtf8("Head-Y"), QString::fromUtf8("160"));
  PREF.addKeyVal(sec, QString::fromUtf8("Head-Z"), QString::fromUtf8("50"));
  QString v;
  if(ui.CapLeds->isChecked()){
    v = QString::fromUtf8("yes");
  }else{
    v = QString::fromUtf8("no");
  }
  PREF.addKeyVal(sec, QString::fromUtf8("Active"), v);
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
  PREF.addKeyVal(sec, QString::fromUtf8("Model-type"), QString::fromUtf8("Clip"));
  PREF.addKeyVal(sec, QString::fromUtf8("Clip-Y1"), QString::number(ui.ClipB->value()));
  PREF.addKeyVal(sec, QString::fromUtf8("Clip-Y2"), 
    QString::number(ui.ClipB->value() + ui.ClipC->value()));
  PREF.addKeyVal(sec, QString::fromUtf8("Clip-Z1"), QString::number(ui.ClipA->value()));
  PREF.addKeyVal(sec, QString::fromUtf8("Clip-Z2"), 
    QString::number(ui.ClipD->value() - ui.ClipA->value()));
  if(ui.ClipLeft->isChecked()){
    PREF.addKeyVal(sec, QString::fromUtf8("Head-X"), QString::fromUtf8("85"));
  }else{
    PREF.addKeyVal(sec, QString::fromUtf8("Head-X"), QString::fromUtf8("-85"));
  }
  PREF.addKeyVal(sec, QString::fromUtf8("Head-Y"), QString::fromUtf8("85"));
  PREF.addKeyVal(sec, QString::fromUtf8("Head-Z"), QString::fromUtf8("140"));
  
  QString v;
  if(ui.ClipLeds->isChecked()){
    v = QString::fromUtf8("yes");
  }else{
    v = QString::fromUtf8("no");
  }
  PREF.addKeyVal(sec, QString::fromUtf8("Active"), v);
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
  PREF.addKeyVal(sec, QString::fromUtf8("Model-type"), QString::fromUtf8("SinglePoint"));
  QString v;
  if(ui.SinglePtLeds->isChecked()){
    v = QString::fromUtf8("yes");
  }else{
    v = QString::fromUtf8("no");
  }
  PREF.addKeyVal(sec, QString::fromUtf8("Active"), v);
}


CapTweaking::CapTweaking(const QString &section, QWidget *parent) : QWidget(parent), currentSection(section),
  initializing(true)
{
  ui.setupUi(this);
  QString val;
  if(PREF.getKeyVal(currentSection, QString::fromUtf8("Head-Y"), val))
    ui.CapHy->setValue(val.toFloat());
  if(PREF.getKeyVal(currentSection, QString::fromUtf8("Head-Z"), val))
    ui.CapHz->setValue(val.toFloat());
  initializing = false;
}

CapTweaking::~CapTweaking()
{
}

void CapTweaking::on_CapHy_valueChanged(int val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, QString::fromUtf8("Head-Y"), val);
  PREF.announceModelChange();
}

void CapTweaking::on_CapHz_valueChanged(int val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, QString::fromUtf8("Head-Z"), val);
  PREF.announceModelChange();
}

ClipTweaking::ClipTweaking(const QString &section, QWidget *parent) : QWidget(parent), currentSection(section),
  initializing(true)
{
  ui.setupUi(this);
  QString val;
  if(PREF.getKeyVal(currentSection, QString::fromUtf8("Head-X"), val)){
    float fval = val.toFloat();
    ui.ClipHx->setValue(fabsf(fval));
    if(fval >= 0){
      ui.ClipLeft->setChecked(true);
    }else{
      ui.ClipRight->setChecked(true);
    }
  }
  if(PREF.getKeyVal(currentSection, QString::fromUtf8("Head-Y"), val))
    ui.ClipHy->setValue(val.toFloat());
  if(PREF.getKeyVal(currentSection, QString::fromUtf8("Head-Z"), val))
    ui.ClipHz->setValue(val.toFloat());
  initializing = false;
}


ClipTweaking::~ClipTweaking()
{
}

void ClipTweaking::tweakHx()
{
  if(!initializing){
    int sign = (ui.ClipLeft->isChecked() ? 1 : -1);
    PREF.setKeyVal(currentSection, QString::fromUtf8("Head-X"), ui.ClipHx->value() * sign);
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
    PREF.setKeyVal(currentSection, QString::fromUtf8("Head-Y"), val);
  PREF.announceModelChange();
}

void ClipTweaking::on_ClipHz_valueChanged(int val)
{
  if(!initializing)
    PREF.setKeyVal(currentSection, QString::fromUtf8("Head-Z"), val);
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

#include "moc_ltr_model.cpp"

