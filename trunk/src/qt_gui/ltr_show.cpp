#include <QImage>
#include <QPixmap>
#include <QLabel>
#include <QTimer>
#include <QTime>
#include <QThread>
#include <QPainter>
#include <QSettings>
#include <iostream>
#include <ltr_show.h>
#include <ltr_gui_prefs.h>
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
#include <ltr_server.h>
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
static int frames = 0;
static float fps_buffer[8] ={0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
static int fps_ptr = 0;
static bool camViewEnable = true;

static bool buffer_empty;

extern "C" {
  void new_frame(struct frame_type *frame, void *param);
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


void CaptureThread::run()
{
  QString section = PREF.getCustomSectionTitle();
  char *section_str = ltr_int_my_strdup(section.toAscii().data());
  std::cout<<"Opening section '"<<section_str<<"'"<<std::endl;
  //section_str = NULL;
  prep_main_loop(section_str);
  ltr_shutdown();
  buffer_empty = false;
  w = 0;
  h = 0;
  clean_up();
  free(section_str);
}

void CaptureThread::signal_new_frame()
{
  emit new_frame();
}

LtrGuiForm::LtrGuiForm(const Ui::LinuxtrackMainForm &tmp_gui, ScpForm *s, QSettings &settings)
              : cv(NULL), allowClose(false), main_gui(tmp_gui)
{
  scp = s;
  ui.setupUi(this);
  label = new QWidget();
  cv = new CameraView(label);
  ui.pix_box->addWidget(label);
  ui.pauseButton->setDisabled(true);
  ui.wakeButton->setDisabled(true);
  ui.stopButton->setDisabled(true);
  settings.beginGroup("TrackingWindow");
  camViewEnable = ! settings.value("camera_view", false).toBool();
  bool check3DV = settings.value("3D_view", false).toBool();
  settings.endGroup();
  main_gui.DisableCamView->setCheckState(camViewEnable ? Qt::Unchecked : Qt::Checked);
  main_gui.Disable3DView->setCheckState(check3DV ? Qt::Checked : Qt::Unchecked);
  glw = new Window(ui.tabWidget, main_gui.Disable3DView);
  ui.ogl_box->addWidget(glw);
  ct = new CaptureThread(this);
  timer = new QTimer(this);
  fpsTimer = new QTimer(this);
  stopwatch = new QTime();
  frames = 0;
  connect(timer, SIGNAL(timeout()), this, SLOT(update()));
  connect(fpsTimer, SIGNAL(timeout()), this, SLOT(updateFps()));
  camViewEnable = true;
  connect(&STATE, SIGNAL(stateChanged(ltr_state_type)), this, SLOT(stateChanged(ltr_state_type)));
  connect(main_gui.DisableCamView, SIGNAL(stateChanged(int)), 
          this, SLOT(disableCamView_stateChanged(int)));
  connect(main_gui.Disable3DView, SIGNAL(stateChanged(int)), 
          this, SLOT(disable3DView_stateChanged(int)));
}

void LtrGuiForm::updateFps()
{
  int msec = stopwatch->restart();
  if(msec > 0){
    fps_buffer[fps_ptr] = 1000 * frames / msec;
    fps_ptr = (fps_ptr + 1) & 7;
    frames = 0;
  }
}

LtrGuiForm::~LtrGuiForm()
{
  if(running){
    ltr_shutdown();
    ct->wait(1000);
  }
  delete glw;
}

void LtrGuiForm::StorePrefs(QSettings &settings)
{
  bool camEna = (main_gui.DisableCamView->checkState() == Qt::Checked) ? true : false;
  bool tdEna = (main_gui.Disable3DView->checkState() == Qt::Checked) ? true : false;
  settings.beginGroup("TrackingWindow");
  settings.setValue("camera_view", camEna);
  settings.setValue("3D_view", tdEna);
  settings.endGroup();
}


void new_frame(struct frame_type *frame, void *param)
{
  struct mmap_s *mmm = (struct mmap_s*)param;
  struct ltr_comm *com = (ltr_comm*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  ltr_int_get_camera_update(&(com->heading), &(com->pitch), &(com->roll), 
                            &(com->tx), &(com->ty), &(com->tz), &(com->counter));
  ltr_int_unlockSemaphore(mmm->sem);
  if(cnt == 0){
    ltr_recenter();
  }
  ++cnt;
  ++frames;
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
  fpsTimer->start(250);
  stopwatch->start();
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
  fpsTimer->stop();
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
  float fps_mean = 0.0f;
  int i;
  for(i = 0; i < 8; ++i){
    fps_mean += fps_buffer[i];
  }
  int fps = fps_mean / 8.0;
  ui.status->setText(QString("%1.frame @ %2 fps").arg(cnt).arg(fps, 4));
  if(buffer_empty){
    return;
  }
  if(img0 != NULL){
    cv->redraw(img0);
    buffer_empty = true;
  }
}

void LtrGuiForm::stateChanged(ltr_state_type current_state)
{
  switch(current_state){
    case STOPPED:
    case ERROR:
      trackerStopped();
      break;
    case INITIALIZING:
    case RUNNING:
      trackerRunning();
      break;
    case PAUSED:
      trackerPaused();
      break;
    default:
      break;
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
