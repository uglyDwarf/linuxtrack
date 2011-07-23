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

#include <linuxtrack.h>
#include <ipc_utils.h>
#include <unistd.h>

QImage *img0;
QPixmap *pic;
QWidget *label;
QVector<QRgb> colors;

static unsigned char *buffer0 = NULL;
static unsigned char *buffer1 = NULL;
static unsigned int w = 0;
static unsigned int h = 0;
static ScpForm *scp;
static bool running = false;
static int cnt = 0;
static bool camViewEnable = true;

static bool buffer_empty;

extern "C" {
  void frame_callback(struct frame_type *frame, void *param);
}

static CaptureThread *ct = NULL;

CaptureThread::CaptureThread(LtrGuiForm *p): QThread(), parent(p)
{
}


void clean_up()
{
  unsigned char *tmp;
  if(buffer0 != NULL){
    tmp = buffer0;
    buffer0 = NULL;
    free(tmp);
  }
  if(buffer1 != NULL){
    tmp = buffer1;
    buffer1 = NULL;
    free(tmp);
  }
  if(img0 != NULL){
    QImage *tmp_i = img0;
    img0 = NULL;
    delete tmp_i;
  }
}

void state_changed(void *param)
{
  struct mmap_s *mmm = (struct mmap_s*)param;
  struct ltr_comm *com = (struct ltr_comm*)mmm->data;
  if(mmm->data != NULL){
    ltr_int_lockSemaphore(mmm->sem);
    com->state = ltr_int_get_tracking_state();
    ltr_int_unlockSemaphore(mmm->sem);
  }
}

void main_loop(char *section)
{
  bool recenter = false;

  char *com_file = ltr_int_get_com_file_name();
  struct mmap_s mmm;
  if(!ltr_int_mmap_file(com_file, sizeof(struct ltr_comm), &mmm)){
    ltr_int_log_message("Couldn't mmap file!!!\n");
    return;
  }
  
  ltr_int_register_cbk(frame_callback, (void*)&mmm, state_changed, (void*)&mmm);
  if(ltr_int_init(section) != 0){
    ltr_int_log_message("Not initialized!\n");
    ltr_int_unmap_file(&mmm);
    return;
  }
  struct ltr_comm *com = (struct ltr_comm*)mmm.data;
  ltr_int_lockSemaphore(mmm.sem);
  com->cmd = NOP_CMD;
  ltr_int_unlockSemaphore(mmm.sem);
  bool break_flag = false;
  while(!break_flag){
    if((com->cmd != NOP_CMD) || com->recenter){
      ltr_int_lockSemaphore(mmm.sem);
      ltr_cmd cmd = (ltr_cmd)com->cmd;
      com->cmd = NOP_CMD;
      recenter = com->recenter;
      com->recenter = false;
      ltr_int_unlockSemaphore(mmm.sem);
      switch(cmd){
        case RUN_CMD:
          ltr_int_wakeup();
          break;
        case PAUSE_CMD:
          ltr_int_suspend();
          break;
        case STOP_CMD:
          ltr_int_shutdown();
          break_flag = true;
          break;
        default:
          //defensive...
          break;
      }
    }
    if(recenter){
      recenter = false;
      ltr_int_recenter();
    }
    usleep(100000);  //ten times per second...
  }
  while(com->state != DOWN){
    usleep(100000);
  }
  ltr_int_unmap_file(&mmm);
}




void CaptureThread::run()
{
  semaphore_p sem;
  int res = ltr_int_server_running_already((char *)"ltr_server.lock", &sem, true);
  if(res == 0){
    ltr_init(NULL);
    main_loop(NULL);
    ltr_int_closeSemaphore(sem);
  }
  buffer_empty = false;
  w = 0;
  h = 0;
  clean_up();
}

void CaptureThread::signal_new_frame()
{
  emit new_frame();
}

LtrGuiForm::LtrGuiForm(const Ui::LinuxtrackMainForm &tmp_gui, ScpForm *s) 
              : cv(NULL), allowClose(false), main_gui(tmp_gui)
{
  scp = s;
  ui.setupUi(this);
  label = new QWidget();
  cv = new CameraView(label);
  ui.pix_box->addWidget(label);
  ui.pauseButton->setDisabled(true);
  ui.wakeButton->setDisabled(true);
//  ui.stopButton->setDisabled(true);
  glw = new Window(ui.tabWidget, main_gui.Disable3DView);
  ui.ogl_box->addWidget(glw);
  ct = new CaptureThread(this);
  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(update()));
  camViewEnable = true;
  connect(&STATE, SIGNAL(trackerStopped()), this, SLOT(trackerStopped()));
  connect(&STATE, SIGNAL(trackerRunning()), this, SLOT(trackerRunning()));
  connect(&STATE, SIGNAL(trackerPaused()), this, SLOT(trackerPaused()));
  connect(main_gui.DisableCamView, SIGNAL(stateChanged(int)), 
          this, SLOT(disableCamView_stateChanged(int)));
  connect(main_gui.Disable3DView, SIGNAL(stateChanged(int)), 
          this, SLOT(disable3DView_stateChanged(int)));
}

LtrGuiForm::~LtrGuiForm()
{
  if(running){
    ltr_shutdown();
    ct->wait(1000);
  }
  delete glw;
}


void frame_callback(struct frame_type *frame, void *param)
{
  (void) param;
  if(cnt == 0){
    ltr_recenter();
  }
  ++cnt;
  
  scp->updatePitch(ltr_int_orig_pose.pitch);
  scp->updateRoll(ltr_int_orig_pose.roll);
  scp->updateYaw(ltr_int_orig_pose.heading);
  scp->updateX(ltr_int_orig_pose.tx);
  scp->updateY(ltr_int_orig_pose.ty);
  scp->updateZ(ltr_int_orig_pose.tz);
  
  if((w != frame->width) || (h != frame->height)){
    w = frame->width;
    h = frame->height;
    clean_up();
    buffer0 = (unsigned char*)ltr_int_my_malloc(h * w);
    memset(buffer0, 0, h * w);
    buffer1 = (unsigned char*)ltr_int_my_malloc(h * w);
    memset(buffer1, 0, h * w);
    img0 = new QImage(buffer0, w, h, w, QImage::Format_Indexed8);
    colors.clear();
    for(int col = 0; col < 256; ++col){
      colors.push_back(qRgb(col, col, col));
    }
    img0->setColorTable(colors);
  }
  if((frame->bitmap != NULL) && camViewEnable){
    //this means that buffer is full
    memcpy(buffer0, buffer1, w * h);
    buffer_empty = false;
    frame->bitmap = NULL;
  }
  if(buffer_empty){
    memset(buffer1, 0, w * h);
    frame->bitmap = buffer1;
  }
  return;
}


void LtrGuiForm::on_startButton_pressed()
{
  ct->start();
  timer->start(50);
}

void LtrGuiForm::on_recenterButton_pressed()
{
  ltr_recenter();
}

void LtrGuiForm::on_pauseButton_pressed()
{
  ltr_suspend();
}

void LtrGuiForm::on_wakeButton_pressed()
{
  ltr_wakeup();
}


void LtrGuiForm::on_stopButton_pressed()
{
  timer->stop();
  if(ltr_shutdown() == 0){
    ct->wait(1000);
  }
}

void LtrGuiForm::disableCamView_stateChanged(int state)
{
  if(state == Qt::Checked){
    camViewEnable = false;
  }else{
    camViewEnable = true;
  }
}

void LtrGuiForm::disable3DView_stateChanged(int state)
{
  if(state == Qt::Checked){
    glw->close_widget();
  }else{
    glw->prepare_widget();
  }
}

void LtrGuiForm::update()
{
  ui.statusbar->showMessage(QString("").setNum(cnt) + ". frame");
  if(buffer_empty){
    return;
  }
  if(img0 != NULL){
    cv->redraw(img0);
    buffer_empty = true;
  }
}

void LtrGuiForm::trackerStopped()
{
  running = false;
  ui.startButton->setDisabled(false);
  ui.pauseButton->setDisabled(true);
  ui.wakeButton->setDisabled(true);
//  ui.stopButton->setDisabled(true);
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

void LtrGuiForm::closeEvent(QCloseEvent *event)
{
  if(allowClose){
    event->accept();
  }else{
    event->ignore();
  }
}

void LtrGuiForm::allowCloseWindow()
{
  allowClose = true;
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
  if((image != NULL) && (running)){
    QPainter painter(this);
    painter.drawImage(QPoint(0, 0), *image);
    painter.end();
  }
}
