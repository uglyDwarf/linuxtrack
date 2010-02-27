#include <QImage>
#include <QPixmap>
#include <QLabel>
#include <QTimer>
#include <iostream>
#include <ltr_gui.h>
#include <cal.h>
#include <utils.h>
#include <pref_global.h>
#include <iostream>

QImage *img;
QPixmap *pic;
QLabel *label;
QTimer *timer;

LtrGuiForm::LtrGuiForm()
 {
     ui.setupUi(this);
     img = new QImage(720, 576, QImage::Format_RGB888);
     pic = new QPixmap(720, 576);
     pic->fill();
     label = new QLabel();
     label->setPixmap(*pic);
     ui.pix_box->addWidget(label);
     timer = new QTimer();
     connect(timer, SIGNAL(timeout()), this, SLOT(update()));
     //timer->start(250);
 }

extern "C" {
  int frame_callback(struct camera_control_block *ccb, struct frame_type *frame);
}

int frame_callback(struct camera_control_block *ccb, struct frame_type *frame)
{
  static int cnt = 0;
  cnt++;
  std::cout<<cnt<<". frame"<<std::endl;
  return 0;
}


void LtrGuiForm::on_startButton_pressed()
{
  static struct camera_control_block ccb;
  if(get_device(&ccb) == false){
    log_message("Can't get device category!\n");
    return;
  }
  ccb.mode = operational_3dot;
  ccb.diag = false;
  cal_run(&ccb, frame_callback);
  ui.statusbar->showMessage("Starting!");
}

void LtrGuiForm::on_pauseButton_pressed()
{
  cal_suspend();
  ui.statusbar->showMessage("Paused!");
}

void LtrGuiForm::on_wakeButton_pressed()
{
  cal_wakeup();
  ui.statusbar->showMessage("Waking!");
}

void LtrGuiForm::on_stopButton_pressed()
{
  cal_shutdown();
  ui.statusbar->showMessage("Stopping!");
  label->clear();
  delete pic;
  pic = new QPixmap(320, 200);
  pic->fill();
  label->setPixmap(*pic);
}

void LtrGuiForm::update()
{
  static int cnt = 0;
  cnt++;
  ui.statusbar->showMessage(QString("").setNum(cnt*250) + "ms");
  
}


