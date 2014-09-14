#ifndef KEYB__H
#define KEYB__H

#include <QObject>
#include <QKeySequence>

class shortcut : public QObject
{
  Q_OBJECT
 public:
  shortcut();
  ~shortcut();
  bool setShortcut(const QKeySequence &s);
  void activate(bool pressed);
 signals:
  void activated(bool pressed);
};

#endif