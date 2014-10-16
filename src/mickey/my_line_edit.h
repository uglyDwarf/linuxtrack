#ifndef TEST__H
#define TEST__H

#include <QLineEdit>

class myLineEdit : public QLineEdit
{
  Q_OBJECT
 public:
  myLineEdit(QWidget *parent = 0);
  void setTargetString(QString &res){tgt = &res;};
 private:
  virtual void keyPressEvent(QKeyEvent * event);
  QString *tgt;
};



#endif
