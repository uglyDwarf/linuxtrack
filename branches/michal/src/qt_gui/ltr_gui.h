#include "ui_ltr_gui.h"


class LtrGuiForm : public QMainWindow
{
    Q_OBJECT
  
  public:
    LtrGuiForm();
  private slots:
    void on_startButton_pressed();
    void on_pauseButton_pressed();
    void on_wakeButton_pressed();
    void on_stopButton_pressed();
    void update();
    
  private:
    Ui::Ltr_gui ui;
};
