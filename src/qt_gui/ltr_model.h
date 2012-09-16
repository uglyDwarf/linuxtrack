#ifndef LTR_MODEL__H
#define LTR_MODEL__H

#include <QObject>
#include <QString>
#include "ui_model_creation.h"
#include "ui_model_edit.h"
#include "ui_cap_edit.h"
#include "ui_clip_edit.h"
#include "ui_single_edit.h"
#include "ui_clip_tweaking.h"
#include "ui_cap_tweaking.h"

typedef enum {MDL_1PT, MDL_3PT_CLIP, MDL_3PT_CAP, MDL_FACE} modelType_t;
class Guardian;

class ModelCreate : public QWidget
{
  Q_OBJECT
 public:
  ModelCreate(QWidget *parent = 0);
  ~ModelCreate();
  virtual void show();
 protected:
 signals:
  void ModelCreated(const QString &section);
 private slots:
  void on_CancelButton_pressed();
  void on_CreateButton_pressed();
  void on_Model3PtCap_pressed();
  void on_Model3PtClip_pressed();
  void on_Model1Pt_pressed();
  void on_ModelFace_pressed();
 signals:
  void dump(const QString &sec);
 private:
  void removeEditor();
  void activateEditor(QWidget *editor);
  Ui::ModelCreation ui;
  QRegExpValidator *validator;
  QWidget *modelEditor;
};

class ModelEdit : public QWidget
{
  Q_OBJECT
 public:
  ModelEdit(Guardian *grd, QWidget *parent = 0);
  ~ModelEdit();
  void refresh();
 protected:
 private slots:
  void on_CreateModelButton_pressed();
  void ModelCreated(const QString &section);
  void on_ModelSelector_activated(const QString &text);
 signals:
  void modelSelected(int modelType);
 private:
  Ui::ModelEditForm ui;
  QWidget *modelTweaker;
  //void Connect();
  ModelCreate *mcw;
  QString currentSection;
  bool initializing;
};

class CapEdit : public QWidget
{
  Q_OBJECT
 public:
  CapEdit(QWidget *parent = 0);
  ~CapEdit();
  void refresh();
 public slots:
  void dump(const QString &sec);
 private:
  Ui::CapEditForm ui;
};


class ClipEdit : public QWidget
{
  Q_OBJECT
 public:
  ClipEdit(QWidget *parent = 0);
  ~ClipEdit();
 public slots:
  void dump(const QString &sec);
 private:
  Ui::ClipEditForm ui;
};

class SingleEdit : public QWidget
{
  Q_OBJECT
 public:
  SingleEdit(QWidget *parent = 0);
  ~SingleEdit();
 public slots:
  void dump(const QString &sec);
 private:
  Ui::SingleEditForm ui;
};

class ClipTweaking : public QWidget
{
  Q_OBJECT
 public:
  ClipTweaking(const QString &section, QWidget *parent = 0);
  ~ClipTweaking();
 public slots:
  void on_ClipHx_valueChanged(int val);
  void on_ClipHy_valueChanged(int val);
  void on_ClipHz_valueChanged(int val);
  void on_ClipLeft_toggled();
  void on_ClipRight_toggled();
 private:
  Ui::ClipTweakingForm ui;
  QString currentSection;
  bool initializing;
  void tweakHx();
};

class CapTweaking : public QWidget
{
  Q_OBJECT
 public:
  CapTweaking(const QString &section, QWidget *parent = 0);
  ~CapTweaking();
 public slots:
  void on_CapHy_valueChanged(int val);
  void on_CapHz_valueChanged(int val);
 private:
  Ui::CapTweakingForm ui;
  QString currentSection;
  bool initializing;
};

#endif
