#include "tracker.h"
#include <tracking.h>
#include <pref_int.h>
#include <axis.h>
#include <ltlib.h>
#include <../ltr_srv_master.h>
#include <../ltr_srv_slave.h>
#include <cstdio>
#include "ltr_gui_prefs.h"
#include <iostream>
#include <unistd.h>

Tracker *Tracker::trr = NULL;
char *com_fname = NULL;

class SlaveThread : public QThread{
  public:
    SlaveThread():profile("Default"){};
    void run();
    void setProfile(QString p){profile = p;};
  private:
    QString profile;
};

void SlaveThread::run()
{
  slave(profile.toAscii().data(), com_fname, true);
}


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

static void ltr_int_state_changed(void *param)
{
  (void) param;
  TRACKER.signalStateChange(ltr_int_get_tracking_state());
}

static void ltr_int_new_frame(struct frame_type *frame, void *param)
{
  (void) param;
  static struct frame_type local_frame;
  local_frame.bloblist = frame->bloblist;
  local_frame.width = frame->width;
  local_frame.height = frame->height;
  local_frame.counter = frame->counter;
  
  unsigned char *tmp = frame->bitmap;
  if(local_frame.bitmap != NULL){
    frame->bitmap = local_frame.bitmap;
  }
  if(tmp != NULL){
    local_frame.bitmap = tmp;
  }
  TRACKER.signalNewFrame(&local_frame);
  static pose_t current_pose;
  ltr_int_get_camera_update(&(current_pose.yaw), &(current_pose.pitch), &(current_pose.roll), 
                            &(current_pose.tx), &(current_pose.ty), &(current_pose.tz), 
                            &(current_pose.counter));
  TRACKER.signalNewPose(&current_pose);
}

Tracker::Tracker() : axes(LTR_AXES_T_INITIALIZER), axes_valid(false)
{
  
  master = new MasterThread();
  slave = new SlaveThread();
  ltr_int_set_callback_hooks(ltr_int_new_frame, ltr_int_state_changed);
  connect(master, SIGNAL(finished()), this, SLOT(masterFinished()));
  setProfile(QString("Default"));
  axes_valid = true;
}

Tracker::~Tracker()
{
  
}

void Tracker::signalStateChange(ltr_state_type current_state)
{
  std::cout<<"Changing state to "<<current_state<<std::endl;
  emit stateChangedz(current_state);
}

void Tracker::signalNewFrame(struct frame_type *frame)
{
  //std::cout<<"Signalling new frame!"<<std::endl;
  emit newFrame(frame);
}

void Tracker::signalNewPose(pose_t *pose)
{
  //std::cout<<"Signalling new pose!"<<std::endl;
  static pose_t processed;
  processed = *pose;
  ltr_int_postprocess_axes(axes, &processed);
  emit newPose(pose, &processed);
}

void Tracker::setProfile(QString p)
{
  axes_valid = false;
  ltr_int_close_axes(&axes);
  PREF.setCustomSection(p);
  ltr_int_init_axes(&axes);
  axes_valid = true;
}

void Tracker::start(QString &section)
{
  (void) section;
  masterShouldRun = true;
  master->start();
  com_fname = ltr_int_init_helper(NULL, false); //Like ltr_init, but doesn't invoke master...
  slave->start();
}

void Tracker::pause()
{
  ltr_suspend();
}

void Tracker::wakeup()
{
  ltr_wakeup();
}

void Tracker::recenter()
{
  ltr_recenter();
}

void Tracker::stop()
{
  masterShouldRun = false;
  ltr_shutdown();
}

void Tracker::masterFinished()
{
  if(masterShouldRun){
    master->start();
  }
}

bool Tracker::axisChangeEnabled(axis_t axis, bool enabled)
{
  ltr_int_set_axis_bool_param(axes, axis, AXIS_ENABLED, enabled);
  emit axisChanged(axis, AXIS_ENABLED);
  return true; 
}

bool Tracker::axisChange(axis_t axis, axis_param_t elem, float val)
{
  bool res = ltr_int_set_axis_param(axes, axis, elem, val);
  emit axisChanged(axis, elem);
  return res;
}

bool Tracker::axisGetEnabled(axis_t axis)
{
  return ltr_int_get_axis_bool_param(axes, axis, AXIS_ENABLED);
}

float Tracker::axisGet(axis_t axis, axis_param_t elem)
{
  return ltr_int_get_axis_param(axes, axis, elem);
}

float Tracker::axisGetValue(axis_t axis, float val)
{
  return ltr_int_val_on_axis(axes, axis, val);
}

bool Tracker::axisIsSymetrical(axis_t axis)
{
  return ltr_int_is_symetrical(axes, axis);
}


