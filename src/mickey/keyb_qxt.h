#ifndef KEYB_QXT__H
#define KEYB_QXT__H

#include <QxtGlobalShortcut>

class shortcutPimpl : public QObject
{
  Q_OBJECT
  public:
   shortcutPimpl(const QKeySequence &s)
   {
     sc = new QxtGlobalShortcut(s, this);
     QObject::connect(sc, SIGNAL(activated()), this, SIGNAL(activated()));
   };
   ~shortcutPimpl(){delete sc;};
   bool setShortcut(const QKeySequence &s){return sc->setShortcut(s);};
  private:
   QxtGlobalShortcut *sc;
  signals:
   void activated();
};

#endif
