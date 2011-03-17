#include <QPainter>

#include "scview.h"
#include "ltr_axis.h"
#include <math.h>
#include <iostream>

SCView::SCView(LtrAxis *a, QWidget *parent)
  : QWidget(parent), axis(a), px(0.0), timer(NULL)
{  
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);
  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(update()));
  timer->start(50);
}

SCView::~SCView()
{
  timer->stop();
  delete timer;
}

void SCView::redraw()
{
  update();
}

QSize SCView::sizeHint() const
{
  return QSize(320, 180);
}

QSize SCView::minimumSizeHint() const
{
  return QSize(320, 180);
}   


static int spline(LtrAxis *a, QPointF points[], int num_points)
{
  float x;
  float k = 2.0f / (num_points - 1);
  for(int i = 0; i < num_points; ++i){
    x = -1.0f + k * i;
    points[i].rx() = x;
    points[i].ry() =  fabs(a->getValue(x * a->getLimits()));
  }
  return 0;
}

static float spline(LtrAxis *a, float x)
{
  return fabs(a->getValue(x));
}


//Should be odd...
static const int spline_points = 101;
static QPointF points[spline_points];

void SCView::paintEvent(QPaintEvent * /* event */)
{
  QSize sz = size();
  float h = sz.height() - 1;//Why, oh why?
  float w = floor((sz.width() / 2) - 1);
  float max_f = (axis->getLFactor() > axis->getRFactor()) ? 
                 axis->getLFactor() : axis->getRFactor();
  
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
  float ny = h - fabs(spline(axis, px * axis->getLimits()) * (h / max_f));
  painter.drawLine(QLineF(nx, ny - 5, nx, ny + 5));
  painter.drawLine(QLineF(nx - 5, ny, nx + 5, ny));
  painter.end();
}

void SCView::movePoint(float new_x)
{
  px = new_x;
}

