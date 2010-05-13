#ifndef SCVIEW__H
#define SCVIEW__H

#include <QWidget>

class LtrAxis;

class SCView : public QWidget
{
     Q_OBJECT
 public:
  SCView(LtrAxis *a, QWidget *parent = 0);
  QSize sizeHint() const;
  QSize minimumSizeHint() const;
  void movePoint(float new_x);
 public slots:
  void redraw();
 protected:
  void paintEvent(QPaintEvent *event);

 private:
  LtrAxis *axis;
  float px;
};



#endif