#include <QtGui>

 #include "glwidget.h"
 #include "window.h"
#include <ltlib_int.h>
#include <iostream>
Window::Window(QWidget *t, QCheckBox *b) : tab(t), control(b), constructed(false)
{
  dynamic_cast<QTabWidget*>(tab)->setTabEnabled(1, false);
  prepare_widget();
}

void Window::prepare_widget()
{
  if(control->checkState() == Qt::Checked){
    return;
  }
  if(!constructed){
    control->setEnabled(false);
    dynamic_cast<QTabWidget*>(tab)->setTabEnabled(1, false);
    glWidget = new GLWidget;
    mainLayout = new QHBoxLayout;
    mainLayout->addWidget(glWidget);
    setLayout(mainLayout);
    
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update_pic()));
    connect(glWidget, SIGNAL(ready()), this, SLOT(start_widget()));
  }
}

void Window::close_widget()
{
  if(constructed){
    control->setEnabled(false);
    timer->stop();
    disconnect(timer, SIGNAL(timeout()), this, SLOT(update_pic()));
    disconnect(glWidget, SIGNAL(ready()), this, SLOT(start_widget()));
    dynamic_cast<QTabWidget*>(tab)->setTabEnabled(1, false);
    delete mainLayout;
    delete glWidget;
    constructed = false;
    control->setEnabled(true);
  }
}

void Window::start_widget()
{
  timer->start(20);
  dynamic_cast<QTabWidget*>(tab)->setTabEnabled(1, true);
  constructed = true;
  control->setEnabled(true);
}

void Window::update_pic()
{
  float h,p,r;
  float x,y,z;
  unsigned int counter;
  if(ltr_int_get_camera_update(&h, &p, &r, &x, &y, &z, &counter) != -1){
    glWidget->setXRotation(p);
    glWidget->setYRotation(h);
    glWidget->setZRotation(r);
    glWidget->setXTrans(x/1000.0);
    glWidget->setYTrans(y/1000.0);
    glWidget->setZTrans(z/1000.0);
    glWidget->updateGL();
  }
}

