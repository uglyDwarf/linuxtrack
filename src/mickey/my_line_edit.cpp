
#include "my_line_edit.h"
#include <iostream>
#include <QKeyEvent>

myLineEdit::myLineEdit(QWidget *parent) : QLineEdit(parent), tgt(NULL)
{
}

void myLineEdit::keyPressEvent(QKeyEvent * event)
{
  QString seq = QString();
  Qt::KeyboardModifiers mods = event->modifiers();
  if(mods & Qt::ShiftModifier){
    seq += QString::fromUtf8("Shift+");
  }
  if(mods & Qt::ControlModifier){
    seq += QString::fromUtf8("Ctrl+");
  }
  if(mods & Qt::AltModifier){
    seq += QString::fromUtf8("Alt+");
  }
  if(mods & Qt::MetaModifier){
    seq += QString::fromUtf8("Meta+");
  }
  int k = event->key();
  //std::cout<<"Key event -> '"<<k<<std::endl;
   
  switch(k){
    case 0:
    case Qt::Key_unknown:
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Alt:
    case Qt::Key_Meta:
      break;
    default:
      seq += QKeySequence(event->key()).toString(QKeySequence::NativeText);
      setText(seq);
      if(tgt != NULL){
        *tgt = seq; 
      }
      //std::cout<<"Key event -> '"<<seq.toUtf8().constData()<<"' !"<<k<<std::endl;
      break;
  }
  //QLineEdit::keyPressEvent(event);
}

#include "moc_my_line_edit.cpp"

