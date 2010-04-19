#include <QPainter>

#include "scview.h"
#include <math.h>
#include <iostream>

SCView::SCView(struct axis_def *a, QWidget *parent)
  : QWidget(parent), axis(a), px(0.0)
{  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);
}

void SCView::redraw()
{
  update();
}

QSize SCView::sizeHint() const
{
  return QSize(360, 180);
}

QSize SCView::minimumSizeHint() const
{
  return QSize(360, 180);
}   


static int spline(struct axis_def *a, QPointF points[], int num_points)
{
  float x;
  float k = 2.0f / (num_points - 1);
  for(int i = 0; i < num_points; ++i){
    x = -1.0f + k * i;
    points[i].rx() = x;
    points[i].ry() =  fabs(val_on_axis(a, x * get_limits(a)));
  }
  return 0;
}

static float spline(struct axis_def *a, float x)
{
  return fabs(val_on_axis(a, x));
}


//Should be odd...
static const int spline_points = 101;
static QPointF points[spline_points];

void SCView::paintEvent(QPaintEvent * /* event */)
{
  QSize sz = size();
  float h = sz.height() - 1;//Why, oh why?
  float w = floor((sz.width() / 2) - 1);
  float max_f = (get_lmult(axis) > get_rmult(axis)) ? get_lmult(axis) : get_rmult(axis);
  
  spline(axis, points, spline_points);
  float x,y;
  for(int i = 0; i < spline_points; ++i){
    x = points[i].x();
    y = points[i].y();
    points[i].rx() = (x + 1.0f) * w;
    points[i].ry() = h - y * (h / max_f);
  }
  QPainter painter(this);
  painter.setPen(Qt::black);
  painter.drawPolyline(points, spline_points);
  painter.setPen(Qt::red);
  float nx = w + w * px;
  float ny = h - fabs(spline(axis, px * get_limits(axis)) * (h / max_f));
  painter.drawLine(QLineF(nx, ny - 5, nx, ny + 5));
  painter.drawLine(QLineF(nx - 5, ny, nx + 5, ny));
  painter.end();
}

void SCView::movePoint(float new_x)
{
  px = new_x;
  redraw();
}

