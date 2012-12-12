#ifndef LTR_SHOW__H
#define LTR_SHOW__H

#include "ui_ltr_gui.h"
#include "ui_ltr.h"
#include <linuxtrack.h>
#include <cal.h>
#include <QThread>
#include <QCloseEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QVBoxLayout>

#include "window.h"
#include "buffering.h"

class LtrGuiForm;
class QSettings;

class CameraView : public QWidget
{
  Q_OBJECT
 public:
  CameraView(QWidget *parent = 0);
  void redraw();
 private:
  QGraphicsScene *scene;
  QGraphicsView *view;
  QGraphicsPixmapItem *item;
  QVBoxLayout *layout;
};

class LtrGuiForm : public QWidget
{
   Q_OBJECT
  public:
   LtrGuiForm(const Ui::LinuxtrackMainForm &tmp_gui, QSettings &settings);
   ~LtrGuiForm();
   void allowCloseWindow();
   void StorePrefs(QSettings &settings);
  public slots:
   void update();
   void updateFps();
    
  private slots:
   void on_startButton_pressed();
   void on_recenterButton_pressed();
   void on_pauseButton_pressed();
   void on_wakeButton_pressed();
   void on_stopButton_pressed();
   void disableCamView_stateChanged(int state);
   void disable3DView_stateChanged(int state);
   void stateChanged(int current_state);
   void newFrameDelivered(struct frame_type *frame);
  protected:
   void closeEvent(QCloseEvent *event);
  private:
   Ui::Ltr_gui ui;
   Window *glw;
   QTimer *timer;
   QTimer *fpsTimer;
   QTime *stopwatch;
   CameraView *cv;
   bool allowClose;
   float fps;
   const Ui::LinuxtrackMainForm &main_gui;
   void trackerStopped();
   void trackerRunning();
   void trackerPaused();
};

#endif
