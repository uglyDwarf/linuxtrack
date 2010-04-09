#ifndef WINDOW_H
 #define WINDOW_H

 #include <QWidget>
 #include <QTimer>
 
 class GLWidget;

 class Window : public QWidget
 {
     Q_OBJECT

 public:
     Window();

 private slots:
  void update_pic();
 private:
     GLWidget *glWidget;
     QTimer *timer;
 };

 #endif
