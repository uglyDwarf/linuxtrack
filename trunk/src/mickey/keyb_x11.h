#ifndef KEYB_X11__h
#define KEYB_X11__h

#include <QX11Info>
#include <QKeySequence>
#include <QAbstractEventDispatcher>
#include <QMutex>
#include <map>
#include <X11/Xlib.h>

typedef std::pair<KeyCode, unsigned int> keyPair_t;

class shortcutPimpl : public QObject
{
  Q_OBJECT
  public:
   shortcutPimpl();
   ~shortcutPimpl();
   bool setShortcut(const QKeySequence &s);
   bool unsetShortcut();
   void activate(){emit activated();};
  signals:
   void activated();
  private:
   Display *display;
   Window window;
   QKeySequence seq;
   KeyCode code;
   unsigned int modifiers;
   static bool errorEncountered;
   static QString errMsg;
   bool shortcutSet;
   static void installFilter();
   static void uninstallFilter();
   static QAbstractEventDispatcher::EventFilter prevFilter;
   static bool eventFilter(void *message);
   static std::map<keyPair_t, shortcutPimpl*> shortcutHash;
   static int my_x_errhandler(Display* display, XErrorEvent *event);
};

#endif

