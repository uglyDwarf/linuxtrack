#ifndef SCVIEW__H
#define SCVIEW__H

#include "spline.h"
#include "axis.h"
#include <QWidget>

class SCView : public QWidget
 {
     Q_OBJECT
 public:
  SCView(const axis_def &a, QWidget *parent = 0);
  QSize sizeHint() const;
  QSize minimumSizeHint() const;

 public slots:
  void redraw();
 protected:
  void paintEvent(QPaintEvent *event);

 private:
  const axis_def &axis;
 };



#endif