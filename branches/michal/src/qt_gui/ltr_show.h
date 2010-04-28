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
  void setRunning();
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
   void setStopped();
   void setRunning();
   void setPaused();
  public slots:
   void update();
    
  private slots:
   void on_startButton_pressed();
   void on_recenterButton_pressed();
   void on_pauseButton_pressed();
   void on_wakeButton_pressed();
   void on_stopButton_pressed();
    
  private:
   Ui::Ltr_gui ui;
   Window *glw;
};
