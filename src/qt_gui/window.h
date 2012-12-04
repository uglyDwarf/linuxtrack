#ifndef WINDOW_H
 #define WINDOW_H

 #include <QWidget>
 #include <QHBoxLayout>
 #include <QTimer>
 #include <QCheckBox>
 #include "tracker.h"
 
 class GLWidget;

 class Window : public QWidget
 {
  Q_OBJECT

 public:
  Window(QWidget *t, QCheckBox *b);
  ~Window();
  void prepare_widget();
  void close_widget();
 private slots:
  void update_pic();
  void start_widget();
  void newPose(pose_t *raw, pose_t *unfiltered, pose_t *processed);
 private:
  GLWidget *glWidget;
  QTimer *timer;
  QWidget *tab;
  QHBoxLayout *mainLayout;
  QCheckBox *control;
  bool constructed;
  bool inConstruction;
  pose_t pose;
 };

 #endif
