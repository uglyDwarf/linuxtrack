#ifndef WINDOW_H
 #define WINDOW_H

 #include <QWidget>
 #include <QHBoxLayout>
 #include <QTimer>
 #include <QCheckBox>
 
 class GLWidget;

 class Window : public QWidget
 {
  Q_OBJECT

 public:
  Window(QWidget *t, QCheckBox *b);
  void prepare_widget();
  void close_widget();
 private slots:
  void update_pic();
  void start_widget();
 private:
  GLWidget *glWidget;
  QTimer *timer;
  QWidget *tab;
  QHBoxLayout *mainLayout;
  QCheckBox *control;
  bool constructed;
 };

 #endif
