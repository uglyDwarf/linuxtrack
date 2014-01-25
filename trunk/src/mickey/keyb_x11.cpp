/*
TODO:
Think through handling for more than one request for the same key.
Reporting problems to user.
*/

#include "keyb_x11.h"
#include <X11/Xproto.h>
#include <cstdio>



QAbstractEventDispatcher::EventFilter shortcutPimpl::prevFilter = NULL;
std::map<std::pair<KeyCode, unsigned int>, shortcutPimpl*> shortcutPimpl::shortcutHash;
bool shortcutPimpl::errorEncountered;
QString shortcutPimpl::errMsg;

void shortcutPimpl::installFilter()
{
  if(shortcutHash.empty()){
    prevFilter = QAbstractEventDispatcher::instance()->setEventFilter(eventFilter);
    printf("Handler installed!\n");
  }
}

void shortcutPimpl::uninstallFilter()
{
  if(shortcutHash.empty()){
    QAbstractEventDispatcher::instance()->setEventFilter(prevFilter);
    printf("Handler removed!\n");
  }
}

bool shortcutPimpl::eventFilter(void *message)
{
  XEvent *event = (XEvent*)message;
  
  if(event->type == KeyPress){
    XKeyEvent *key = (XKeyEvent*) event;
    keyPair_t kp(key->keycode, key->state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask));
    std::map<keyPair_t, shortcutPimpl*>::iterator i = shortcutHash.find(kp);
    if(i != shortcutHash.end()){
      i->second->activate();
      return true;
    }
  }
  if(prevFilter != NULL){
    return prevFilter(message);
  }
  return false;
}


static int (*prev_x_errhandler)(Display* display, XErrorEvent* event);

#define ERR_MSG_SIZE 1024

int shortcutPimpl::my_x_errhandler(Display* display, XErrorEvent *event)
{
  if((event->request_code == X_GrabKey)||(event->request_code == X_UngrabKey)){
    char msg[ERR_MSG_SIZE];
    //to be 100% sure there is a place for ending NULL, I sub 1 from ERR_MSG_SIZE
    XGetErrorText(display, event->error_code, msg, ERR_MSG_SIZE-1);
    printf("X Error: %s\n", msg);
    errMsg = QString::fromUtf8(msg);
    errorEncountered = true;
  }
  return 0;
}


shortcutPimpl::shortcutPimpl() : code(0), modifiers(0), shortcutSet(false)
{
  display = QX11Info::display();
  window = QX11Info::appRootWindow();
}

shortcutPimpl::~shortcutPimpl()
{
  if(shortcutSet){
    unsetShortcut();
  }
}


unsigned int getModifiers(int key)
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


bool shortcutPimpl::setShortcut(const QKeySequence &s)
{
  if(s.isEmpty()){
    printf("Empty key sequence!\n");
    return false;
  }
  QKeySequence key = QKeySequence(s[0] & (~Qt::KeyboardModifierMask));
  modifiers = getModifiers(s[0]);
  KeySym sym = XStringToKeysym(qPrintable(key.toString()));
  if(sym == NoSymbol){
    printf("Unknown symbol!\n");
    return false;
  }
  code = XKeysymToKeycode(display, sym);
  if(code == 0){
    printf("Unknown code!\n");
    return false;
  }
  
  //verify the hotkey doesn't exist already
  keyPair_t kp(code, modifiers);
  std::map<keyPair_t, shortcutPimpl*>::iterator i = shortcutHash.find(kp);
  if(i != shortcutHash.end()){
    printf("The hotkey is taken already!\n");
    return false;
  } 
  
  seq = s;
  prev_x_errhandler = XSetErrorHandler(my_x_errhandler);
  //Flush stuff, so we don't get false errors... Paranoia? ;)
  XSync(display, false);
  errorEncountered = false;
  //Grab the key with possible NumLock and ScrollLock
  XGrabKey(display, code, modifiers, window, true, GrabModeAsync, GrabModeAsync);
  //Numlock
  XGrabKey(display, code, modifiers | Mod2Mask, window, true, GrabModeAsync, GrabModeAsync);
  //ScrollLock
  XGrabKey(display, code, modifiers | Mod5Mask, window, true, GrabModeAsync, GrabModeAsync);
  //NumLock + ScrollLock
  XGrabKey(display, code, modifiers | Mod2Mask | Mod5Mask, window, true, GrabModeAsync, GrabModeAsync);
  XSync(display, false);
  XSetErrorHandler(prev_x_errhandler);
  if(errorEncountered){
    printf("Encountered error!\n");
    unsetShortcut();
  }else{
    printf("Shortcut set!\n");
    shortcutSet = true;
    installFilter();
    shortcutHash.insert(std::pair<std::pair<KeyCode, unsigned int>, shortcutPimpl*>
      (keyPair_t(code, modifiers), this));
  }
  return !errorEncountered;
}

bool shortcutPimpl::unsetShortcut()
{
  shortcutHash.erase(keyPair_t(code, modifiers));
  uninstallFilter();
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


