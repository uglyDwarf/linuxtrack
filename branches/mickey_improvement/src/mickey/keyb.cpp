#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include "keyb.h"
#ifndef DARWIN
  #include "keyb_x11.h"
#endif

#include <cstdio>
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
