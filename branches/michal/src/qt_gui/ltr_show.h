#ifndef LTR_SHOW__H
#define LTR_SHOW__H

#include "ui_ltr_gui.h"

#include <QThread>
#include "window.h"

class ScpForm;
class LtrGuiForm;

class CaptureThread : public QThread
{
  Q_OBJECT
 public:
  CaptureThread(LtrGuiForm *p);
  void run();
  void signal_new_frame();
 signals:
  void new_frame();  
 private:
  LtrGuiForm *parent;
  
};

class LtrGuiForm : public QMainWindow
{
   Q_OBJECT
  public:
   LtrGuiForm(ScpForm *s);
   ~LtrGuiForm();
  public slots:
   void update();
    
  private slots:
   void on_startButton_pressed();
   void on_recenterButton_pressed();
   void on_pauseButton_pressed();
   void on_wakeButton_pressed();
   void on_stopButton_pressed();

   void trackerStopped();
   void trackerRunning();
   void trackerPaused();

  private:
   Ui::Ltr_gui ui;
   ScpForm *sens;
   Window *glw;
};

#endif