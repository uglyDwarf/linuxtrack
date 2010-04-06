#include <QtGui>

 #include "glwidget.h"
 #include "window.h"

 Window::Window()
 {
     glWidget = new GLWidget;

     xSlider = createAngleSlider();
     ySlider = createAngleSlider();
     zSlider = createAngleSlider();

     txSlider = createSlider();
     tySlider = createSlider();
     tzSlider = createSlider();
     
     connect(xSlider, SIGNAL(valueChanged(int)), glWidget, SLOT(setXRotation(int)));
     connect(glWidget, SIGNAL(xRotationChanged(int)), xSlider, SLOT(setValue(int)));
     connect(ySlider, SIGNAL(valueChanged(int)), glWidget, SLOT(setYRotation(int)));
     connect(glWidget, SIGNAL(yRotationChanged(int)), ySlider, SLOT(setValue(int)));
     connect(zSlider, SIGNAL(valueChanged(int)), glWidget, SLOT(setZRotation(int)));
     connect(glWidget, SIGNAL(zRotationChanged(int)), zSlider, SLOT(setValue(int)));
     connect(txSlider, SIGNAL(valueChanged(int)), glWidget, SLOT(setXTrans(int)));
     connect(tySlider, SIGNAL(valueChanged(int)), glWidget, SLOT(setYTrans(int)));
     connect(tzSlider, SIGNAL(valueChanged(int)), glWidget, SLOT(setZTrans(int)));

     QHBoxLayout *mainLayout = new QHBoxLayout;
     mainLayout->addWidget(glWidget);
     mainLayout->addWidget(xSlider);
     mainLayout->addWidget(ySlider);
     mainLayout->addWidget(zSlider);
     mainLayout->addWidget(txSlider);
     mainLayout->addWidget(tySlider);
     mainLayout->addWidget(tzSlider);
     setLayout(mainLayout);

     setWindowTitle(tr("Hello GL"));
 }

 QSlider *Window::createAngleSlider()
 {
     QSlider *slider = new QSlider(Qt::Vertical);
     slider->setRange(-180 * 16, 180 * 16);
     slider->setSingleStep(16);
     slider->setPageStep(15 * 16);
     slider->setTickInterval(15 * 16);
     slider->setTickPosition(QSlider::TicksRight);
     slider->setValue(0);
     return slider;
 }

 QSlider *Window::createSlider()
 {
     QSlider *slider = new QSlider(Qt::Vertical);
     slider->setRange(-500, 500);
     slider->setSingleStep(1);
     slider->setPageStep(10);
     slider->setTickInterval(25);
     slider->setTickPosition(QSlider::TicksRight);
     slider->setValue(0);
     return slider;
 }
