#include "tracker.h"
#include <tracking.h>
#include <pref.hpp>
#include <axis.h>
#include <ltlib.h>
#include <../ltr_srv_master.h>
#include <../ltr_srv_slave.h>
#include <cstdio>
#include "ltr_gui_prefs.h"
#include <iostream>
#include <unistd.h>
#include <QApplication>
#include <QMessageBox>

Tracker *Tracker::trr = NULL;
char *com_fname = NULL;

class MasterThread : public QThread{
  public:
    void run();
};

void MasterThread::run()
{
  master(false);
}

Tracker& Tracker::trackerInst()
{
  if(trr == NULL){
    trr = new Tracker();
  }
  return *trr;
}

static void ltr_int_new_slave(char *name)
{
  TRACKER.signalNewSlave(name);
}

static void ltr_int_state_changed(void *param)
{
  (void) param;
  TRACKER.signalStateChange(ltr_int_get_tracking_state());
}

static buffering buf(3);
static bool initBuffers = true;
static void ltr_int_new_frame(struct frame_type *frame, void *param)
{
  (void) param;
  static struct frame_type local_frame;
  local_frame.bloblist = frame->bloblist;
  local_frame.width = frame->width;
  local_frame.height = frame->height;
  local_frame.counter = frame->counter;
  
  if(initBuffers){
    buf.resizeBuffers(frame->width, frame->height);
    initBuffers = false;
  }
  
  if(frame->bitmap != NULL){
    buf.bufferWritten();
  }
  buffer *b;
  if(buf.writeBuffer(&b)){
    frame->bitmap = b->getBuffer();
  }else{
    frame->bitmap = NULL;
  }
  
  TRACKER.signalNewFrame(&local_frame);
  static pose_t current_pose;
  ltr_int_get_camera_update(&(current_pose.yaw), &(current_pose.pitch), &(current_pose.roll), 
                            &(current_pose.tx), &(current_pose.ty), &(current_pose.tz), 
                            &(current_pose.counter));
  TRACKER.signalNewPose(&current_pose);
}

buffering *Tracker::getBuffers()
{
  return &buf;
}


Tracker::Tracker() : axes(LTR_AXES_T_INITIALIZER), axes_valid(false), currentProfile("Default"), common_ff(0.0)
{
  if(!ltr_int_gui_lock(true)){
    QMessageBox::warning(NULL, "Linuxtrack", "Another linuxtrack gui is running already!",
                         QMessageBox::Ok);
  }
  master = new MasterThread();
  ltr_int_set_callback_hooks(ltr_int_new_frame, ltr_int_state_changed, ltr_int_new_slave);
  setProfile(QString("Default"));
  axes_valid = true;
}

Tracker::~Tracker()
{
  if(master->isRunning()){
    request_shutdown();
    master->wait();
  }
}

void Tracker::signalStateChange(ltr_state_type current_state)
{
  emit stateChanged(current_state);
}

void Tracker::signalNewFrame(struct frame_type *frame)
{
  emit newFrame(frame);
}

void Tracker::signalNewPose(pose_t *pose)
{
  static pose_t processed;
  static pose_t unfiltered;
  processed = *pose;
  ltr_int_postprocess_axes(axes, &processed, &unfiltered);
  //std::cout<<"TRACKER: "<<pose->pitch<<" "<<unfiltered.pitch<<" "<<processed.pitch<<std::endl;
  emit newPose(pose, &unfiltered, &processed);
}


void Tracker::signalNewSlave(const char *name)
{
  ltr_axes_t tmp_axes = LTR_AXES_T_INITIALIZER;
  ltr_int_init_axes(&tmp_axes, name);
  for(int i = PITCH; i <= TZ; ++i){
    change(name, i, AXIS_ENABLED, ltr_int_get_axis_bool_param(tmp_axes, (axis_t)i, AXIS_ENABLED)?1.0:0.0);
    for(int j = AXIS_DEADZONE; j <= AXIS_FILTER; ++j){
      change(name, i, j, ltr_int_get_axis_param(tmp_axes, (axis_t)i, (axis_param_t)j));
    }
  }
  ltr_int_close_axes(&tmp_axes);
}


void Tracker::setProfile(QString p)
{
  axes_valid = false;
  ltr_int_close_axes(&axes);
  //PREF.setCustomSection(p);
  currentProfile = p;
  PREF.getProfileSection(currentProfile, profileSection);
  //std::cout<<"Set profile "<<currentProfile.toStdString()<<" - "<<p.toStdString()<<std::endl;
  ltr_int_init_axes(&axes, currentProfile.toStdString().c_str());
  
  common_ff = 1.0;
  int i;
  for(i = PITCH; i <= TZ; ++i){
    ffs[i] = ltr_int_get_axis_param(axes, (axis_t)i, AXIS_FILTER);
    if(common_ff > ffs[i]){
      common_ff = ffs[i];
    }
  }
  for(i = PITCH; i <= TZ; ++i){
    ffs[i] -= common_ff;
  }
  axes_valid = true;
  emit initAxes();
}

void Tracker::fromDefault()
{
  ltr_int_axes_from_default(&axes);
  emit initAxes();
}

void Tracker::start(QString &section)
{
  (void) section;
  initBuffers = true;
  buf.init();
  master->start();
}

void Tracker::pause()
{
  suspend_cmd();
}

void Tracker::wakeup()
{
  wakeup_cmd();
}

void Tracker::recenter()
{
  recenter_cmd();
}

void Tracker::stop()
{
  request_shutdown();
}

bool Tracker::axisChangeEnabled(axis_t axis, bool enabled)
{
  ltr_int_set_axis_bool_param(axes, axis, AXIS_ENABLED, enabled);
  emit axisChanged(axis, AXIS_ENABLED);
  change(profileSection.toStdString().c_str(), axis, AXIS_ENABLED, enabled?1.0:0.0);
  return true; 
}

static float limit_ff(float val)
{
  if(val < 0.0) return 0.0;
  if(val > 2.0) return 2.0;
  return val;
}

bool Tracker::axisChange(axis_t axis, axis_param_t elem, float val)
{
  if(elem == AXIS_FILTER){
    ffs[axis] = val;
    val = limit_ff(val + common_ff);
  }
  bool res = ltr_int_set_axis_param(axes, axis, elem, val);
  emit axisChanged(axis, elem);
  change(profileSection.toStdString().c_str(), axis, elem, val);
  return res;
}

bool Tracker::axisGetEnabled(axis_t axis)
{
  return ltr_int_get_axis_bool_param(axes, axis, AXIS_ENABLED);
}

float Tracker::axisGet(axis_t axis, axis_param_t elem)
{
  if(elem == AXIS_FILTER){
    return ffs[axis];
  }else{
    return ltr_int_get_axis_param(axes, axis, elem);
  }
}

float Tracker::axisGetValue(axis_t axis, float val)
{
  return ltr_int_val_on_axis(axes, axis, val);
}

bool Tracker::axisIsSymetrical(axis_t axis)
{
  return ltr_int_is_symetrical(axes, axis);
}

bool Tracker::setCommonFilterFactor(float c_f)
{
  common_ff = c_f;
  bool res = true;
  float val;
  for(int i = PITCH; i <= TZ; ++i){
    val = limit_ff(ffs[i] + common_ff);
    res &= ltr_int_set_axis_param(axes, (axis_t)i, AXIS_FILTER, val);
    emit axisChanged(i, AXIS_FILTER);
    change(profileSection.toStdString().c_str(), i, AXIS_FILTER, val);
  }
  emit setCommonFF(c_f);
  return res;
}

float Tracker::getCommonFilterFactor()
{
  return common_ff;
}


