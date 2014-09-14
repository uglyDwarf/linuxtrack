#ifndef HOTKEY__H
#define HOTKEY__H

#include <QWidget>
#include <QSettings>
#include "ui_hotkey.h"
#include "keyb.h"

class HotKey : public QWidget{
 Q_OBJECT
 public:
  HotKey(const QString iPrefId, int iHotKeyId, QWidget *parent = 0);
  ~HotKey(){delete s;};
  bool setHotKey(const QString &newHK);
  void getHotKey(QString &hk) const;
 signals:
  void activated(int id, bool pressed);
  void newHotKey(const QString &hotKeyId, const QString &hk);  
 private slots:
  void shortcutActivated(bool pressed);
  void on_AssignButton_pressed();
 private:
  QString prefId;
  int hotKeyId;
  QString hotKey;
  Ui::HotKeySetup ui;
  shortcut *s;
};


#endif
