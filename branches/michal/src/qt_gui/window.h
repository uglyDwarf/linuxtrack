#ifndef WINDOW_H
 #define WINDOW_H

 #include <QWidget>

 class QSlider;

 class GLWidget;

 class Window : public QWidget
 {
     Q_OBJECT

 public:
     Window();

 private:
     QSlider *createSlider();
     QSlider *createAngleSlider();

     GLWidget *glWidget;
     QSlider *xSlider;
     QSlider *ySlider;
     QSlider *zSlider;
     QSlider *txSlider;
     QSlider *tySlider;
     QSlider *tzSlider;
 };

 #endif
