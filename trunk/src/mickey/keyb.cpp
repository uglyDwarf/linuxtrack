#include "keyb.h"
#include "keyb_qxt.h"

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
