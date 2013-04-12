#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include "keyb.h"
#ifndef DARWIN
  #include "keyb_x11.h"
#endif

shortcut::shortcut()
{
  key = new shortcutPimpl();
  QObject::connect(key, SIGNAL(activated()), this, SIGNAL(activated()));
}

shortcut::~shortcut()
{
  delete key;
}

bool shortcut::setShortcut(const QKeySequence &s)
{
  return key->setShortcut(s);
}
