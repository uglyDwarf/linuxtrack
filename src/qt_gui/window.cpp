#include <QtGui>

 #include "glwidget.h"
 #include "window.h"
#include <ltlib.h>
#include <iostream>

Window::Window(QWidget *t, QCheckBox *b) : glWidget(NULL), tab(t), control(b), constructed(false)
{
  dynamic_cast<QTabWidget*>(tab)->setTabEnabled(1, false);
  prepare_widget();
}

Window::~Window()
{
  close_widget();
}

void Window::prepare_widget()
{
  if(control->checkState() == Qt::Checked){
    return;
  }
  if(!constructed){
    inConstruction = true;
    control->setEnabled(false);
    dynamic_cast<QTabWidget*>(tab)->setTabEnabled(1, false);
    glWidget = new GLWidget;
    mainLayout = new QHBoxLayout;
    mainLayout->addWidget(glWidget);
    setLayout(mainLayout);
    
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update_pic()));
    connect(glWidget, SIGNAL(ready()), this, SLOT(start_widget()));
    connect(&TRACKER, SIGNAL(newPose(pose_t *, pose_t *)), this, SLOT(newPose(pose_t *, pose_t *)));
  }
}

void Window::close_widget()
{
  if(constructed){
    disconnect(timer, SIGNAL(timeout()), this, SLOT(update_pic()));
    disconnect(glWidget, SIGNAL(ready()), this, SLOT(start_widget()));
    control->setEnabled(false);
    timer->stop();
    dynamic_cast<QTabWidget*>(tab)->setTabEnabled(1, false);
    constructed = false;
    control->setEnabled(true);
  }
  delete mainLayout;
  if(glWidget != NULL){
    delete glWidget;
  }
}

void Window::start_widget()
{
  timer->start(20);
  dynamic_cast<QTabWidget*>(tab)->setTabEnabled(1, true);
  inConstruction = false;
  constructed = true;
  control->setEnabled(true);
}

void Window::newPose(pose_t *raw, pose_t *processed)
{
  (void) raw;
  glWidget->setXRotation(processed->pitch);
  glWidget->setYRotation(processed->yaw);
  glWidget->setZRotation(processed->roll);
  glWidget->setXTrans(processed->tx/1000.0);
  glWidget->setYTrans(processed->ty/1000.0);
  glWidget->setZTrans(processed->tz/1000.0);
}

void Window::update_pic()
{
  glWidget->updateGL();
}

