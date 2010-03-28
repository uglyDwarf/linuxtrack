#ifndef LTR_MODEL__H
#define LTR_MODEL__H

#include <QObject>
#include "ui_model_creation.h"

class ModelCreate : public QWidget
{
  Q_OBJECT
 public:
  ModelCreate(QWidget *parent = 0);
  ~ModelCreate();
 protected:
 private slots:
  void on_CancelButton_pressed();
  void on_CreateButton_pressed();
  void on_Model3PtCap_pressed();
  void on_Model3PtClip_pressed();
  private:
  Ui::ModelCreation ui;
};

#endif