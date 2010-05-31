#ifndef WINDOW_H
 #define WINDOW_H

 #include <QWidget>
 #include <QTimer>
 
 class GLWidget;

 class Window : public QWidget
 {
     Q_OBJECT

 public:
     Window(QWidget *t);

 private slots:
  void update_pic();
  void start_widget();
 private:
     GLWidget *glWidget;
     QTimer *timer;
     QWidget *tab;
 };

 #endif
