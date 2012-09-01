#ifndef LTR_MODEL__H
#define LTR_MODEL__H

#include <QObject>
#include <QString>
#include "ui_model_creation.h"
#include "ui_model_edit.h"
#include "ui_cap_edit.h"
#include "ui_clip_edit.h"
#include "ui_single_edit.h"

class ModelCreate : public QWidget
{
  Q_OBJECT
 public:
  ModelCreate(QWidget *parent = 0);
  ~ModelCreate();
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
  private:
  Ui::ModelCreation ui;
};

class ModelEdit : public QWidget
{
  Q_OBJECT
 public:
  ModelEdit(QWidget *parent = 0);
  ~ModelEdit();
  void refresh();
 protected:
 private slots:
  void on_CreateModelButton_pressed();
  void ModelCreated(const QString &section);
  void on_ModelSelector_activated(const QString &text);
  private:
  Ui::ModelEditForm ui;
  QWidget *modelEditor;
  //void Connect();
  ModelCreate *mcw;
  QString currentSection;
  bool initializing;
};

class CapEdit : public QWidget
{
  Q_OBJECT
 public:
  CapEdit(const QString &section, QWidget *parent = 0);
  ~CapEdit();
  void refresh();
 protected:
 private slots:
  void on_CapA_valueChanged(double val);
  void on_CapB_valueChanged(double val);
  void on_CapC_valueChanged(double val);
  void on_CapHy_valueChanged(double val);
  void on_CapHz_valueChanged(double val);
  void on_CapLeds_stateChanged(int state);
  private:
  Ui::CapEditForm ui;
  //void Connect();
  QString currentSection;
  bool initializing;
};


class ClipEdit : public QWidget
{
  Q_OBJECT
 public:
  ClipEdit(const QString &section, QWidget *parent = 0);
  ~ClipEdit();
 private slots:
  void on_ClipA_valueChanged(double val);
  void on_ClipB_valueChanged(double val);
  void on_ClipC_valueChanged(double val);
  void on_ClipD_valueChanged(double val);
  void on_ClipHx_valueChanged(double val);
  void on_ClipHy_valueChanged(double val);
  void on_ClipHz_valueChanged(double val);
  void on_ClipLeds_stateChanged(int state);
  private:
  Ui::ClipEditForm ui;
  //void Connect();
  QString currentSection;
  bool initializing;
};

class SingleEdit : public QWidget
{
  Q_OBJECT
 public:
  SingleEdit(const QString &section, QWidget *parent = 0);
  ~SingleEdit();
 private slots:
  void on_SinglePtLeds_stateChanged(int state);
  private:
  Ui::SingleEditForm ui;
  QString currentSection;
  bool initializing;
};

#endif
