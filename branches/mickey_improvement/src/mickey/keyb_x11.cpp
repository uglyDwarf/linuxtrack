/*
TODO:
Think through handling for more than one request for the same key.
Reporting problems to user.
*/

#include <X11/Xproto.h>
#include "keyb_x11.h"
#include "keyb.h"
#include <cstdio>

#ifdef QT5_OVERRIDES
  #include <xcb/xcb.h>
#endif

static shortcutHash_t shortcutHash;
static bool errorEncountered;
static QString errMsg;
static Display *display = NULL;
static Window window;

static bool grabKeyX(Display *display, Window &window, KeyCode code, unsigned int modifiers);
static bool ungrabKeyX(Display *display, Window &window, KeyCode code, unsigned int modifiers);

#ifdef QT5_OVERRIDES

static hotKeyFilter *hkFilter = NULL;

bool hotKeyFilter::nativeEventFilter(const QByteArray & eventType, void * message, long * result)
{
  Q_UNUSED(result);
  if(eventType == "xcb_generic_event_t"){
    xcb_generic_event_t* ev = static_cast<xcb_generic_event_t *>(message);
    xcb_key_press_event_t *keyEvent = NULL;
    unsigned int keyCode = 0;
    unsigned int modifiers = 0;
    //127 is mask allowing to process events regardless of their source
    bool pressed = ((ev->response_type & 127) == XCB_KEY_PRESS);
    bool released = ((ev->response_type & 127) == XCB_KEY_RELEASE);
    if(pressed || released){
      keyEvent = static_cast<xcb_key_press_event_t *>(message);
      keyCode = keyEvent->detail;
      modifiers = 0;
      if(keyEvent->state & XCB_MOD_MASK_1){
        modifiers |= Mod1Mask;
      }
      if(keyEvent->state & XCB_MOD_MASK_CONTROL){
        modifiers |= ControlMask;
      }
      if(keyEvent->state & XCB_MOD_MASK_4){
        modifiers |= Mod4Mask;
      }
      if(keyEvent->state & XCB_MOD_MASK_SHIFT){
        modifiers |= ShiftMask;
      }
      keyPair_t kp(keyCode, modifiers & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask));
      shortcutHash_t::iterator i = shortcutHash.find(kp);
      if(i != shortcutHash.end()){
        i->second->activate(pressed);
        return true;
      }
    }
  }
  return false;
}

#else //QT < 5.0

static QAbstractEventDispatcher::EventFilter prevFilter = NULL;

static bool eventFilter(void *message)
{
  XEvent *event = (XEvent*)message;
  bool pressed = event->type == KeyPress;
  bool released = event->type == KeyRelease;
  if(pressed || released){
    XKeyEvent *key = (XKeyEvent*) event;
    keyPair_t kp(key->keycode, key->state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask));
    shortcutHash_t::iterator i = shortcutHash.find(kp);
    if(i != shortcutHash.end()){
      i->second->activate(pressed);
      return true;
    }
  }
  if(prevFilter != NULL){
    return prevFilter(message);
  }
  return false;
}
#endif

static void installFilter()
{
#ifdef QT5_OVERRIDES
  hkFilter = new hotKeyFilter();
  QAbstractEventDispatcher::instance()->installNativeEventFilter(hkFilter);
#else
  if(prevFilter == NULL){
    prevFilter = QAbstractEventDispatcher::instance()->setEventFilter(eventFilter);
    //printf("Handler installed!\n");
  }
#endif  
}

static void uninstallFilter()
{
#ifdef QT5_OVERRIDES
  QAbstractEventDispatcher::instance()->removeNativeEventFilter(hkFilter);
  delete hkFilter;
  hkFilter = NULL;
#else
  if(prevFilter != NULL){
    QAbstractEventDispatcher::instance()->setEventFilter(prevFilter);
    //printf("Handler removed!\n");
  }
#endif
}


static int (*prev_x_errhandler)(Display* display, XErrorEvent* event);

#define ERR_MSG_SIZE 1024

static int my_x_errhandler(Display* display, XErrorEvent *event)
{
  if((event->request_code == X_GrabKey)||(event->request_code == X_UngrabKey)){
    char msg[ERR_MSG_SIZE];
    //to be 100% sure there is a place for ending NULL, I sub 1 from ERR_MSG_SIZE
    XGetErrorText(display, event->error_code, msg, ERR_MSG_SIZE-1);
    //printf("X Error: %s\n", msg);
    errMsg = QString::fromUtf8(msg);
    errorEncountered = true;
  }
  return 0;
}

static unsigned int getModifiers(int key)
{
  int mod = Qt::NoModifier;
  mod = key & (Qt::KeyboardModifierMask);
  unsigned int modifiers = 0;
  
  modifiers |= (mod & Qt::ShiftModifier)? ShiftMask : 0;
  modifiers |= (mod & Qt::ControlModifier)? ControlMask : 0;
  modifiers |= (mod & Qt::AltModifier)? Mod1Mask : 0;
  modifiers |= (mod & Qt::MetaModifier)? Mod4Mask : 0;
  //Mod2Mask => NumLock!
  //Mod5Mask => ScrollLock!
  return modifiers;
}

static bool removeIdFromHash(shortcut* shortcutId, keyPair_t *kp = NULL)
{
  bool res = false;
  shortcutHash_t::iterator i;
  for(i = shortcutHash.begin(); i != shortcutHash.end(); /* empty */){
    if(i->second == shortcutId){
      //printf("Removing id %p\n", shortcutId);
      if(kp != NULL){
	*kp = i->first;
      }
      shortcutHash.erase(i++);
      res = true;
    }else{
      ++i;
    }
  }
  return res;
}

static bool translateSequence(const QKeySequence &s, KeyCode &code, unsigned int &modifiers)
{
  QKeySequence key = QKeySequence(s[0] & (~Qt::KeyboardModifierMask));
  modifiers = getModifiers(s[0]);
  KeySym sym = XStringToKeysym(qPrintable(key.toString()));
  if(sym == NoSymbol){
    //printf("Unknown symbol!\n");
    return false;
  }
  code = XKeysymToKeycode(display, sym);
  if(code == 0){
    //printf("Unknown code!\n");
    return false;
  }
  return true;
}

bool setShortCut(const QKeySequence &s, shortcut* shortcutId)
{
  if(display == NULL){
    display = QX11Info::display();
    window = QX11Info::appRootWindow();
  }
  removeIdFromHash(shortcutId);
  if(s.isEmpty()){
    //printf("Empty key sequence!\n");
    return false;
  }
  unsigned int modifiers;
  KeyCode code;
  if(!translateSequence(s, code, modifiers)){
    return false;
  }
  //verify the hotkey doesn't exist already
  keyPair_t kp(code, modifiers);
  shortcutHash_t::iterator i = shortcutHash.find(kp);
  if(i != shortcutHash.end()){
    //printf("The hotkey is taken already!\n");
    return false;
  } 
  
  if(!grabKeyX(display, window, code, modifiers)){
    //printf("Problem setting shortcut!\n");
    return false;
  }

  //printf("Shortcut set!\n");
  bool needInstallFilter = shortcutHash.empty();
  shortcutHash.insert(std::pair<std::pair<KeyCode, unsigned int>, shortcut *>
    (keyPair_t(code, modifiers), shortcutId));
  if(needInstallFilter){
    installFilter();
  }
  return true;
}

bool unsetShortcut(shortcut* id)
{
  keyPair_t kp;
  if(!removeIdFromHash(id, &kp)){
    return false;
  }
  KeyCode code = kp.first;
  unsigned int modifiers = kp.second;
  
  if(shortcutHash.empty()){
    uninstallFilter();
  }
  return ungrabKeyX(display, window, code, modifiers);
}

static bool grabKeyX(Display *display, Window &window, KeyCode code, unsigned int modifiers)
{
  prev_x_errhandler = XSetErrorHandler(my_x_errhandler);
  //Flush...
  XSync(display, false);
  errorEncountered = false;
  //No modifiers
  XGrabKey(display, code, modifiers, window, true, GrabModeAsync, GrabModeAsync);
  XSync(display, false);
  if(errorEncountered){
    // nothing to clean up...
    return false;
  }

  //Numlock
  XGrabKey(display, code, modifiers | Mod2Mask, window, true, GrabModeAsync, GrabModeAsync);
  XSync(display, false);
  if(errorEncountered){
    goto num_lock_failed;
  }
    
  //ScrollLock
  XGrabKey(display, code, modifiers | Mod5Mask, window, true, GrabModeAsync, GrabModeAsync);
  XSync(display, false);
  if(errorEncountered){
    goto scroll_lock_failed;
  }

  //NumLock + ScrollLock
  XGrabKey(display, code, modifiers | Mod2Mask | Mod5Mask, window, true, GrabModeAsync, GrabModeAsync);
  XSync(display, false);
  if(!errorEncountered){
    XSetErrorHandler(prev_x_errhandler);
    return true;
  }
  
  //Num+scroll lock failed, clean ScrollLock, num lock and no mod
  XUngrabKey(display, code, modifiers | Mod5Mask, window);
 scroll_lock_failed:
  //Scroll lock failed, clean up Numlock and no mod
  XUngrabKey(display, code, modifiers | Mod2Mask, window);
 num_lock_failed:
  //Num lock failed, clean up no mod
  XUngrabKey(display, code, modifiers, window);

  XSetErrorHandler(prev_x_errhandler);
  return false;
}

static bool ungrabKeyX(Display *display, Window &window, KeyCode code, unsigned int modifiers)
{
  prev_x_errhandler = XSetErrorHandler(my_x_errhandler);
  //Flush stuff, so we don't get false errors... Paranoia? ;)
  XSync(display, false);
  errorEncountered = false;
  XUngrabKey(display, code, modifiers, window);
  //Numlock
  XUngrabKey(display, code, modifiers | Mod2Mask, window);
  //ScrollLock
  XUngrabKey(display, code, modifiers | Mod5Mask, window);
  //NumLock + ScrollLock
  XUngrabKey(display, code, modifiers | Mod2Mask | Mod5Mask, window);
  XSync(display, false);
  XSetErrorHandler(prev_x_errhandler);
  return !errorEncountered;
}

