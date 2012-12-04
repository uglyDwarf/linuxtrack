#include <QPainter>

#include "scview.h"
#include "ltr_axis.h"
#include <math.h>
#include <iostream>

SCView::SCView(LtrAxis *a, QWidget *parent)
  : QWidget(parent), parentWidget(parent), axis(a), px(0.0), timer(NULL)
{
  setBackgroundRole(QPalette::Base);
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  setAutoFillBackground(true);
  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(update()));
  timer->start(50);
  setMinimumSize(400, 100);
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

//QSize SCView::sizeHint() const
//{
//  if(parentWidget){
//    std::cout<<"Size: "<<parentWidget->size().height() <<" "<< parentWidget->size().width() << std::endl;
//    return parentWidget->size();
//  }else{
//    return minimumSizeHint();
//  }
//}

//QSize SCView::minimumSizeHint() const
//{
//  return QSize(320, 180);
//}   


static int spline(LtrAxis *a, QPointF points[], int num_points)
{
  float x, kl, kr, max_k;
  float k = 2.0f / (num_points - 1);
  kl = (a->getLFactor() != 0.0) ? a->getLLimit() / a->getLFactor() : 0.0;
  kr = (a->getRFactor() != 0.0) ? a->getRLimit() / a->getRFactor() : 0.0;
  max_k = kl > kr ? kl : kr;
  for(int i = 0; i < num_points; ++i){
    x = -1.0f + k * i;
    points[i].rx() = x;
    points[i].ry() = fabs(a->getValue(x * max_k));
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
//  float max_f = (axis->getLFactor() > axis->getRFactor()) ? 
//                 axis->getLFactor() : axis->getRFactor();
  float max_f = (axis->getLLimit() > axis->getRLimit()) ? 
                 axis->getLLimit() : axis->getRLimit();
  
  spline(axis, points, spline_points);
  float x,y;
  for(int i = 0; i < spline_points; ++i){
    x = points[i].x();
    y = points[i].y();
    points[i].rx() = (x + 1.0f) * w;
    points[i].ry() = (max_f > 0.0) ? h - y * (h / max_f) : h;
  }
  QPainter painter(this);
  painter.setPen(Qt::black);
  painter.drawPolyline(points, spline_points);
  painter.setPen(Qt::red);
  float kl = (axis->getLFactor() != 0.0) ? axis->getLLimit() / axis->getLFactor() : 0.0;
  float kr = (axis->getRFactor() != 0.0) ? axis->getRLimit() / axis->getRFactor() : 0.0;
  float max_k = kl > kr ? kl : kr;
  float nx = w + w * px/max_k;
  float ny = (max_f != 0.0) ? h - fabs(spline(axis, px) * (h / max_f)) : h;
  painter.drawLine(QLineF(nx, ny - 5, nx, ny + 5));
  painter.drawLine(QLineF(nx - 5, ny, nx + 5, ny));
  painter.drawText(QPoint(100,10), QString("Real: %1").arg(px));
  painter.drawText(QPoint(100,20), QString("Simulated: %1").arg(spline(axis, px)));
  painter.end();
}

void SCView::movePoint(float new_x)
{
  px = new_x;
}

