#include <QPainter>

#include "scview.h"
#include <math.h>
#include <iostream>

SCView::SCView(const axis_def &a, QWidget *parent)
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


static int spline(const axis_def *a, QPointF points[], int num_points)
{
  splines pts;
  float x, fac;
  curve2pts(&(a->curves), &pts);
  float k = 2.0f / (num_points - 1);
  for(int i = 0; i < num_points; ++i){
    x = -1.0f + k * i;
    fac = ((x < 0.0f) ? a->l_factor : a->r_factor);
    points[i].rx() = x;
    points[i].ry() =  fac * fabs(spline_point(&pts, x));
  }
  return 0;
}

static float spline(const axis_def *a, float x)
{
  splines pts;
  float fac;
  curve2pts(&(a->curves), &pts);
  fac = ((x < 0.0f) ? a->l_factor : a->r_factor); 
  return fac * fabs(spline_point(&pts, x));
}


//Should be odd...
static const int spline_points = 101;
static QPointF points[spline_points];

void SCView::paintEvent(QPaintEvent * /* event */)
{
  QSize sz = size();
  float h = sz.height() - 1;//Why, oh why?
  float w = floor((sz.width() / 2) - 1);
  float max_f = (axis.l_factor > axis.r_factor) ? axis.l_factor : axis.r_factor;
  
  spline(&axis, points, spline_points);
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
  float ny = h - fabs(spline(&axis, px) * (h / max_f));
//  painter.drawLine(QLineF(nx - 5, ny - 5, nx + 5, ny + 5));
//  painter.drawLine(QLineF(nx - 5, ny + 5, nx + 5, ny - 5));
  painter.drawLine(QLineF(nx, ny - 5, nx, ny + 5));
  painter.drawLine(QLineF(nx - 5, ny, nx + 5, ny));
  painter.end();
}

void SCView::movePoint(float new_x)
{
  px = new_x;
  redraw();
}

