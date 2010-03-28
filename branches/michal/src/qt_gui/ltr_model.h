#ifndef LTR_MODEL__H
#define LTR_MODEL__H

#include <QObject>
#include "ui_model_creation.h"
#include "ui_ltr.h"

class ModelCreate : public QWidget
{
  Q_OBJECT
 public:
  ModelCreate(QWidget *parent = 0);
  ~ModelCreate();
 protected:
 signals:
  void ModelCreated();
 private slots:
  void on_CancelButton_pressed();
  void on_CreateButton_pressed();
  void on_Model3PtCap_pressed();
  void on_Model3PtClip_pressed();
  void on_Model1Pt_pressed();
  private:
  Ui::ModelCreation ui;
};

class ModelEdit : public QWidget
{
  Q_OBJECT
 public:
  ModelEdit(const Ui::LinuxtrackMainForm &ui);
  ~ModelEdit();
 protected:
 private slots:
  void on_CreateModelButton_pressed();
  void on_CapA_valueChanged(double val);
  void on_CapB_valueChanged(double val);
  void on_CapC_valueChanged(double val);
  void on_CapHy_valueChanged(double val);
  void on_CapHz_valueChanged(double val);
  void on_CapLeds_stateChanged(int state);
  void on_ClipA_valueChanged(double val);
  void on_ClipB_valueChanged(double val);
  void on_ClipC_valueChanged(double val);
  void on_ClipD_valueChanged(double val);
  void on_ClipHx_valueChanged(double val);
  void on_ClipHy_valueChanged(double val);
  void on_ClipHz_valueChanged(double val);
  void on_ClipLeds_stateChanged(int state);
  void on_SinglePtLeds_stateChanged(int state);
  void on_ModelCreated();
  void on_ModelSelector_activated(const QString &text);
  private:
  const Ui::LinuxtrackMainForm &gui;
  void Connect();
  ModelCreate *mcw;
  QString currentSection;
};

#endif