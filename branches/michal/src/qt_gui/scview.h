#ifndef SCVIEW__H
#define SCVIEW__H

#include "axis.h"
#include <QWidget>

class SCView : public QWidget
{
     Q_OBJECT
 public:
  SCView(struct axis_def *a, QWidget *parent = 0);
  QSize sizeHint() const;
  QSize minimumSizeHint() const;
  void movePoint(float new_x);

 public slots:
  void redraw();
 protected:
  void paintEvent(QPaintEvent *event);

 private:
  struct axis_def *axis;
  float px;
};



#endif