#include <QtGui>

 #include "glwidget.h"
 #include "window.h"
#include <ltlib_int.h>
#include <iostream>
Window::Window(QWidget *t) : tab(t)
{
     tab->setEnabled(false);
     glWidget = new GLWidget;
     QHBoxLayout *mainLayout = new QHBoxLayout;
     mainLayout->addWidget(glWidget);
     setLayout(mainLayout);

     timer = new QTimer(this);
     connect(timer, SIGNAL(timeout()), this, SLOT(update_pic()));
     connect(glWidget, SIGNAL(ready()), this, SLOT(start_widget()));
}

void Window::start_widget()
{
  timer->start(20);
  tab->setEnabled(true);
}

void Window::update_pic()
{
  float h,p,r;
  float x,y,z;
  if(ltr_int_get_camera_update(&h, &p, &r, &x, &y, &z) != -1){
    glWidget->setXRotation(-p);
    glWidget->setYRotation(h);
    glWidget->setZRotation(r);
    glWidget->setXTrans(-x/1000.0);
    glWidget->setYTrans(-y/1000.0);
    glWidget->setZTrans(-z/1000.0);
    glWidget->updateGL();
  }
}

