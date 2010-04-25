#include "ui_ltr_gui.h"

#include <QThread>
#include "window.h"

class ScpForm;

class CaptureThread : public QThread
{
  Q_OBJECT
 public:
  void run();
  void signal_new_frame();
 signals:
  void new_frame();  
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
    
  private:
   Ui::Ltr_gui ui;
   Window *glw;
   enum {RUNNING, PAUSED, STOPPED} state;
};
