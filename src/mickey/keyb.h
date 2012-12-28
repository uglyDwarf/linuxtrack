#ifndef KEYB__H
#define KEYB__H

#include <QObject>
#include <QKeySequence>

class shortcutPimpl;

class shortcut : public QObject
{
  Q_OBJECT
 public:
  shortcut(const QKeySequence &s);
  ~shortcut();
  bool setShortcut(const QKeySequence &s);
 signals:
  void activated();
 private:
  shortcutPimpl *key;
};

#endif