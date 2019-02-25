

#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include "keyb.h"
// Must be before the keyb_x11.h otherwise a type mismatch occurs
//   (qmetatype must be included before any header file defining Bool,
//    which in this case is X11/Xlib.h)
#include "moc_keyb.cpp"

#ifndef DARWIN
  #include "keyb_x11.h"
#endif

shortcut::shortcut()
{
}

shortcut::~shortcut()
{
  unsetShortcut(this);
}

bool shortcut::setShortcut(const QKeySequence &s)
{
  //printf("Setting shortcut!\n");
  return setShortCut(s, this);
}

bool shortcut::resetShortcut()
{
  return unsetShortcut(this);
}

void shortcut::activate(bool pressed)
{
  //printf("Firing shortcut!\n");
  emit activated(pressed);
}


