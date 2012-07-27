#ifndef SCVIEW__H
#define SCVIEW__H

#include <QWidget>
#include <QTimer>
#include <axis.h>
#include <ltlib.h>

class SCView : public QWidget
{
     Q_OBJECT
 public:
  SCView(axis_t a, QWidget *parent = 0);
  ~SCView();
//  QSize sizeHint() const;
//  QSize minimumSizeHint() const;
//  void movePoint(float new_x);
 public slots:
  void redraw();
  void newPose(pose_t *raw_pose, pose_t *unfiltered, pose_t *pose);
 protected:
  void paintEvent(QPaintEvent *event);

 private:
  int spline(QPointF points[], int num_points);
  float spline(float x);
  QWidget *parentWidget;
  float rx; //raw value
  float px; //processed value
  float upx; //unfiltered value
  axis_t axis;
  QTimer *timer;
};



#endif
