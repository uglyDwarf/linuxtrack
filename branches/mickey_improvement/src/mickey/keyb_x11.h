#ifndef KEYB_X11__h
#define KEYB_X11__h

#include <QX11Info>
#include <QKeySequence>
#include <QAbstractEventDispatcher>
#include <QMutex>
#include <map>
#include <X11/Xlib.h>

#ifdef QT5_OVERRIDES
  #include <QAbstractNativeEventFilter>
#endif

class shortcut;

typedef std::pair<KeyCode, unsigned int> keyPair_t;
typedef std::map<keyPair_t, shortcut*> shortcutHash_t;

bool setShortCut(const QKeySequence &s, shortcut* id);
bool unsetShortcut(shortcut* id);

#ifdef QT5_OVERRIDES
class hotKeyFilter : public QAbstractNativeEventFilter
{
 protected:
  bool nativeEventFilter(const QByteArray & eventType, void * message, long * result); 
};
#endif
/*
class shortcutPimpl : public QObject
{
  Q_OBJECT
  public:
   ~shortcutPimpl();
   bool setShortcut(const QKeySequence &s, shortcut* id);
   bool unsetShortcut(shortcut* id);
   //void activate(int id){emit activated(id);};
   static shortcutPimpl *createShortcutObject();
  signals:
   void activated(int);
  private:
   shortcutPimpl();
   static void installFilter();
   static void uninstallFilter();
   static QAbstractEventDispatcher::EventFilter prevFilter;
   static bool eventFilter(void *message);
   static shortcutHash_t shortcutHash;
   static int my_x_errhandler(Display* display, XErrorEvent *event);
   static bool grabKeyX(Display *display, Window &window, KeyCode code, unsigned int modifiers);
   static bool ungrabKeyX(Display *display, Window &window, KeyCode code, unsigned int modifiers);
   static bool removeIdFromHash(shortcut* shortcutId, keyPair_t *kp = NULL);
   static bool translateSequence(const QKeySequence &s, KeyCode &code, unsigned int &modifiers);
};
*/

#endif

