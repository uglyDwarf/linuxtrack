#include <QtWidgets>
#include <QTabWidget>

 #include "glwidget.h"
 #include "window.h"
#include <ltlib.h>
#include <iostream>

Window::Window(QWidget *t, QCheckBox *b) : glWidget(NULL), tab(t), mainLayout(NULL), control(b), 
                                           constructed(false) 
                                           
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
    connect(&TRACKER, SIGNAL(newPose(linuxtrack_full_pose_t *, linuxtrack_pose_t *, linuxtrack_pose_t *)), 
             this, SLOT(newPose(linuxtrack_full_pose_t *, linuxtrack_pose_t *, linuxtrack_pose_t *)));
  }
}

void Window::close_widget()
{
  if(constructed){
    disconnect(timer, SIGNAL(timeout()), this, SLOT(update_pic()));
    disconnect(glWidget, SIGNAL(ready()), this, SLOT(start_widget()));
    disconnect(&TRACKER, SIGNAL(newPose(linuxtrack_full_pose_t *, linuxtrack_pose_t *, linuxtrack_pose_t *)), 
                this, SLOT(newPose(linuxtrack_full_pose_t *, linuxtrack_pose_t *, linuxtrack_pose_t *)));

    control->setEnabled(false);
    timer->stop();
    dynamic_cast<QTabWidget*>(tab)->setTabEnabled(1, false);
    constructed = false;
    control->setEnabled(true);
  }
  if(mainLayout != NULL){
    delete mainLayout;
    mainLayout = NULL;
  }
  if(glWidget != NULL){
    GLWidget *tmp = glWidget;
    glWidget = NULL;
    delete tmp;
  }
}

void Window::start_widget()
{
  dynamic_cast<QTabWidget*>(tab)->setTabEnabled(1, true);
  inConstruction = false;
  constructed = true;
  control->setEnabled(true);
}

void Window::newPose(linuxtrack_full_pose_t *raw, linuxtrack_pose_t *unfiltered, linuxtrack_pose_t *processed)
{
  (void) raw;
  (void) unfiltered;
  if(glWidget != NULL){
    glWidget->setXRotation(processed->pitch);
    glWidget->setYRotation(processed->yaw);
    glWidget->setZRotation(processed->roll);
    glWidget->setXTrans(processed->tx/1000.0);
    glWidget->setYTrans(processed->ty/1000.0);
    glWidget->setZTrans(processed->tz/1000.0);
  }
}

void Window::update_pic()
{
  glWidget->updateGL();
}

void Window::stopUpdates()
{
  if(timer){
    timer->stop();
  }
}

void Window::resumeUpdates()
{
  if(timer){
    timer->start(20);
  }
}

#include "moc_window.cpp"


