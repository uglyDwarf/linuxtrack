#include <QImage>
#include <QPixmap>
#include <QLabel>
#include <QTimer>
#include <QThread>
#include <QPainter>
#include <iostream>
#include <ltr_show.h>
#include <cal.h>
#include <utils.h>
#include <pref_global.h>
#include <pref_int.h>
#include <tracking.h>
#include <iostream>
#include <scp_form.h>
#include <ltr_state.h>
#include <string.h>

QImage *img0;
QImage *img1;
QPixmap *pic;
QWidget *label;
QVector<QRgb> colors;

unsigned char *bitmap = NULL;
static unsigned char *qt_bitmap = NULL;
static unsigned char *buffer0 = NULL;
static unsigned char *buffer1 = NULL;
static unsigned int w = 0;
static unsigned int h = 0;
static ScpForm *scp;
static bool running = false;
static int cnt = 0;

extern "C" {
  int frame_callback(struct camera_control_block *ccb, struct frame_type *frame);
}

static CaptureThread *ct = NULL;

CaptureThread::CaptureThread(LtrGuiForm *p): QThread(), parent(p)
{
}

void CaptureThread::run()
{
  static struct camera_control_block ccb;
  if(get_device(&ccb) == false){
    log_message("Can't get device category!\n");
    return;
  }
  init_tracking();
  ccb.diag = false;
  cal_run(&ccb, frame_callback);
}

void CaptureThread::signal_new_frame()
{
  emit new_frame();
}

LtrGuiForm::LtrGuiForm(ScpForm *s) : cv(NULL)
{
  scp = s;
  ui.setupUi(this);
//  label = new QLabel();
  label = new QWidget();
  cv = new CameraView(label);
  ui.pix_box->addWidget(label);
  ui.pauseButton->setDisabled(true);
  ui.wakeButton->setDisabled(true);
  ui.stopButton->setDisabled(true);
  glw = new Window();
  ui.ogl_box->addWidget(glw);
  ct = new CaptureThread(this);
  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(update()));
  timer->start(50);
//  connect(ct, SIGNAL(new_frame()), this, SLOT(update()));
  
  connect(&STATE, SIGNAL(trackerStopped()), this, SLOT(trackerStopped()));
  connect(&STATE, SIGNAL(trackerRunning()), this, SLOT(trackerRunning()));
  connect(&STATE, SIGNAL(trackerPaused()), this, SLOT(trackerPaused()));
}

LtrGuiForm::~LtrGuiForm()
{
  if(running){
    cal_shutdown();
    ct->wait(1000);
  }
  delete glw;
}


int frame_callback(struct camera_control_block *ccb, struct frame_type *frame)
{
  (void) ccb;
  
  if(cnt == 0){
    recenter_tracking();
  }
  ++cnt;
  
  update_pose(frame);
  scp->updatePitch(lt_orig_pose.pitch);
  scp->updateRoll(lt_orig_pose.roll);
  scp->updateYaw(lt_orig_pose.heading);
  scp->updateX(lt_orig_pose.tx);
  scp->updateY(lt_orig_pose.ty);
  scp->updateZ(lt_orig_pose.tz);
  
  if((w != frame->width) || (h != frame->height)){
    w = frame->width;
    h = frame->height;
    if(buffer0 != NULL){
      free(buffer0);
    }
    if(buffer1 != NULL){
      free(buffer1);
    }
    buffer0 = (unsigned char*)my_malloc(h * w);
    memset(buffer0, 0, h * w);
    buffer1 = (unsigned char*)my_malloc(h * w);
    memset(buffer1, 200, h * w);
    img0 = new QImage(buffer0, w, h, w, QImage::Format_Indexed8);
    img1 = new QImage(buffer1, w, h, w, QImage::Format_Indexed8);
    qt_bitmap = buffer0;
    colors.clear();
    for(int col = 0; col < 256; ++col){
      colors.push_back(qRgb(col, col, col));
    }
    img0->setColorTable(colors);
    img1->setColorTable(colors);
  }
  if(bitmap != NULL){
    qt_bitmap = bitmap;
    bitmap = NULL;
  }
  frame->bitmap = qt_bitmap;
  return 0;
}


void LtrGuiForm::on_startButton_pressed()
{
  ct->start();
}

void LtrGuiForm::on_recenterButton_pressed()
{
  recenter_tracking();
}

void LtrGuiForm::on_pauseButton_pressed()
{
  cal_suspend();
}

void LtrGuiForm::on_wakeButton_pressed()
{
  cal_wakeup();
}


void LtrGuiForm::on_stopButton_pressed()
{
  if(cal_shutdown() == 0){
    ct->wait(1000);
  }
}

void LtrGuiForm::update()
{
  ui.statusbar->showMessage(QString("").setNum(cnt) + ". frame");

  if(qt_bitmap == NULL){
    return;
  }
  if(qt_bitmap != buffer0){
    cv->redraw(img1);
    memset(buffer0, 0,  h * w);
    bitmap = buffer0;
  }else{
    cv->redraw(img0);
    memset(buffer1, 0, h * w);
    bitmap = buffer1;
  }
}

void LtrGuiForm::trackerStopped()
{
  running = false;
  ui.startButton->setDisabled(false);
  ui.pauseButton->setDisabled(true);
  ui.wakeButton->setDisabled(true);
  ui.stopButton->setDisabled(true);
  ui.recenterButton->setDisabled(true);
}

void LtrGuiForm::trackerRunning()
{
  running = true;
  ui.startButton->setDisabled(true);
  ui.pauseButton->setDisabled(false);
  ui.wakeButton->setDisabled(true);
  ui.stopButton->setDisabled(false);
  ui.recenterButton->setDisabled(false);
}

void LtrGuiForm::trackerPaused()
{
  running = true;
  ui.startButton->setDisabled(true);
  ui.pauseButton->setDisabled(true);
  ui.wakeButton->setDisabled(false);
  ui.stopButton->setDisabled(false);
  ui.recenterButton->setDisabled(true);
}

CameraView::CameraView(QWidget *parent)
  : QWidget(parent), image(NULL)
{  
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);
}

void CameraView::redraw(QImage *img)
{
  image = img;
  if(size() != img->size()){
    resize(img->size());
  }
  update();
}

void CameraView::paintEvent(QPaintEvent * /* event */)
{
  if(image != NULL){
    QPainter painter(this);
    painter.drawImage(QPoint(0, 0), *image);
    painter.end();
  }
}
