#include "keyb.h"
#include "keyb_qxt.h"

shortcut::shortcut(const QKeySequence &s)
{
  key = new shortcutPimpl(s);
  QObject::connect(key, SIGNAL(activated()), this, SIGNAL(activated()));
}

shortcut::~shortcut()
{
  delete key;
}

