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
 private slots:
  void on_Profiles_currentIndexChanged(const QString &text);
};


#endif
