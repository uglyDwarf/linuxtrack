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
    local_frame.bitmap = NULL;
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

Tracker::Tracker() : axes(LTR_AXES_T_INITIALIZER), axes_valid(false), currentProfile("Default")
{
  if(!ltr_int_gui_lock()){
    QMessageBox::warning(NULL, "Linuxtrack", "Another linuxtrack gui is running already!",
                         QMessageBox::Ok);
  }
  master = new MasterThread();
  ltr_int_set_callback_hooks(ltr_int_new_frame, ltr_int_state_changed);
  setProfile(QString("Default"));
  axes_valid = true;
}

Tracker::~Tracker()
{
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
  processed = *pose;
  ltr_int_postprocess_axes(axes, &processed);
  emit newPose(pose, &processed);
}

void Tracker::setProfile(QString p)
{
  axes_valid = false;
  ltr_int_close_axes(&axes);
  //PREF.setCustomSection(p);
  currentProfile = PREF.getCustomSectionName();
  std::cout<<"Set profile "<<currentProfile.toStdString()<<" - "<<p.toStdString()<<std::endl;
  ltr_int_init_axes(&axes);
  axes_valid = true;
  
  emit axisChanged(PITCH, AXIS_FULL);
  emit axisChanged(ROLL, AXIS_FULL);
  emit axisChanged(YAW, AXIS_FULL);
  emit axisChanged(TX, AXIS_FULL);
  emit axisChanged(TY, AXIS_FULL);
  emit axisChanged(TZ, AXIS_FULL);

}

void Tracker::start(QString &section)
{
  (void) section;
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
  QString section(PREF.getCustomSectionTitle());
  std::cout<<"Section1: "<<section.toStdString()<<std::endl;
  change(section.toStdString().c_str(), axis, AXIS_ENABLED, enabled?1.0:0.0);
  return true; 
}

bool Tracker::axisChange(axis_t axis, axis_param_t elem, float val)
{
  bool res = ltr_int_set_axis_param(axes, axis, elem, val);
  emit axisChanged(axis, elem);
  QString section(PREF.getCustomSectionTitle());
  std::cout<<"Section2: "<<section.toStdString()<<std::endl;
  change(section.toStdString().c_str(), axis, elem, val);
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


