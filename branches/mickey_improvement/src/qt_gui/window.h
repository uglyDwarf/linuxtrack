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
  void stopUpdates();
  void resumeUpdates();
 private slots:
  void update_pic();
  void start_widget();
  void newPose(linuxtrack_full_pose_t *raw, linuxtrack_pose_t *unfiltered, linuxtrack_pose_t *processed);
 private:
  GLWidget *glWidget;
  QTimer *timer;
  QWidget *tab;
  QHBoxLayout *mainLayout;
  QCheckBox *control;
  bool constructed;
  bool inConstruction;
  linuxtrack_pose_t pose;
 };

 #endif
