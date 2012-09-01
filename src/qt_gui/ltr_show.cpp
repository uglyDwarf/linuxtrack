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
#include <pref.hpp>
#include <tracking.h>
#include <iostream>
#include <scp_form.h>
#include <ltr_state.h>
#include <string.h>

#include <linuxtrack.h>
#include <ltr_server.h>
#include <ipc_utils.h>
#include <unistd.h>
#include <tracker.h>

QImage *img0;
QImage *img1;
QImage *current_img = NULL;
//QPixmap *pic;
QWidget *label;
QVector<QRgb> colors;

static unsigned char *buffer0 = NULL;
static unsigned char *buffer1 = NULL;
static unsigned char *current_buffer = NULL;
static unsigned int w = 0;
static unsigned int h = 0;
static ScpForm *scp;
static bool running = false;
static bool camViewEnable = true;
static int cnt = 0;
static int frames = 0;
static float fps_buffer[8] ={0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
static int fps_ptr = 0;

//!!!TBD multithread sync!!!


void clean_up()
{
  unsigned char *tmp;
  if(buffer0 != NULL){
    //to avoid problem is someone is using it...
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



LtrGuiForm::LtrGuiForm(const Ui::LinuxtrackMainForm &tmp_gui, QSettings &settings)
              : cv(NULL), allowClose(false), main_gui(tmp_gui)
{
  ui.setupUi(this);
  label = new QWidget();
  cv = new CameraView(label);
  ui.pix_box->addWidget(label);
  trackerStopped();
  settings.beginGroup("TrackingWindow");
  camViewEnable = ! settings.value("camera_view", false).toBool();
  bool check3DV = settings.value("3D_view", false).toBool();
  settings.endGroup();
  main_gui.DisableCamView->setCheckState(camViewEnable ? Qt::Unchecked : Qt::Checked);
  main_gui.Disable3DView->setCheckState(check3DV ? Qt::Checked : Qt::Unchecked);
  glw = new Window(ui.tabWidget, main_gui.Disable3DView);
  ui.ogl_box->addWidget(glw);
  timer = new QTimer(this);
  fpsTimer = new QTimer(this);
  stopwatch = new QTime();
  frames = 0;
  connect(timer, SIGNAL(timeout()), this, SLOT(update()));
  connect(fpsTimer, SIGNAL(timeout()), this, SLOT(updateFps()));
  camViewEnable = true;
  if(!connect(&TRACKER, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)))){
    std::cout<<"Problem connecting signal1!"<<std::endl;
  }
  if(!connect(&TRACKER, SIGNAL(newFrame(struct frame_type *)), this, SLOT(newFrameDelivered(struct frame_type *)))){
    std::cout<<"Problem connecting signal2!"<<std::endl;
  }
  connect(main_gui.DisableCamView, SIGNAL(stateChanged(int)), 
          this, SLOT(disableCamView_stateChanged(int)));
  connect(main_gui.Disable3DView, SIGNAL(stateChanged(int)), 
          this, SLOT(disable3DView_stateChanged(int)));
}

void LtrGuiForm::newFrameDelivered(struct frame_type *frame)
{
  if(cnt == 0){
    ltr_recenter();
  }
  ++cnt;
  ++frames;
  if((w != frame->width) || (h != frame->height)){
    w = frame->width;
    h = frame->height;
    clean_up();
    buffer0 = (unsigned char*)ltr_int_my_malloc(h * w);
    memset(buffer0, 0, h * w);
    buffer1 = (unsigned char*)ltr_int_my_malloc(h * w);
    memset(buffer1, 0, h * w);
    img0 = new QImage(buffer0, w, h, w, QImage::Format_Indexed8);
    img1 = new QImage(buffer1, w, h, w, QImage::Format_Indexed8);
    colors.clear();
    for(int col = 0; col < 256; ++col){
      colors.push_back(qRgb(col, col, col));
    }
    img0->setColorTable(colors);
    img1->setColorTable(colors);
  }
  if((frame->bitmap != NULL) && camViewEnable){
    current_buffer = frame->bitmap;
    if(frame->bitmap == buffer0){
      current_img = img0;
      frame->bitmap = buffer1;
    }else{
      current_img = img1;
      frame->bitmap = buffer0;
    }
    memset(frame->bitmap, 0, h * w);
  }else{
    frame->bitmap = buffer0;
  }
  return;
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



void LtrGuiForm::on_startButton_pressed()
{
  timer->start(50);
  fpsTimer->start(250);
  stopwatch->start();
  static QString sec("Default");
  TRACKER.start(sec);
}

void LtrGuiForm::on_recenterButton_pressed()
{
  TRACKER.recenter();
}

void LtrGuiForm::on_pauseButton_pressed()
{
  TRACKER.pause();
}

void LtrGuiForm::on_wakeButton_pressed()
{
  TRACKER.wakeup();
}


void LtrGuiForm::on_stopButton_pressed()
{
  TRACKER.stop();
  timer->stop();
  fpsTimer->stop();
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
  cv->redraw();
}

void LtrGuiForm::stateChanged(int current_state)
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

void CameraView::redraw()
{
  update();
}

void CameraView::paintEvent(QPaintEvent * /* event */)
{
  image = current_img;
  if(image == NULL){
    return;
  }
  if(size() != image->size()){
    resize(image->size());
  }
  if((image != NULL)/* && (running)*/){
    QPainter painter(this);
    painter.drawPixmap(QPoint(0, 0), QPixmap::fromImage(*image));
    painter.end();
  }
}


