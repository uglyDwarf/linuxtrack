#ifndef PROFILE_SELECTOR__H
#define PROFILE_SELECTOR__H

#include <QWidget>
#include "ui_profile_selector.h"

class ProfileSetup;

class ProfileSelector : public QWidget
{
 Q_OBJECT
 public:
  ProfileSelector(QWidget *parent = 0);
  ~ProfileSelector();
  void refresh();
 private:
   Ui::ProfileSelectorForm ui;
   ProfileSetup *ps;
   bool initializing;
   bool setCurrentProfile(QString prof);
 public slots:
   bool close();
 private slots:
  void on_Profiles_currentIndexChanged(const QString &text);
  void on_CopyFromDefault_pressed();
  void on_ImportProfile_pressed();
  void on_ExportProfile_pressed();
};


#endif
