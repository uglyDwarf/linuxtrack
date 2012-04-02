#include "tracker.h"
#include <../ltr_srv_master.h>
#include <../ltr_srv_slave.h>
#include <cstdio>

Tracker *Tracker::trr = NULL;

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
  slave(profile.toAscii().data());
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
  //printf("TRACKER::frame_received!\n");
  //TODO - param is ptr to pose_t..., pass it on too
  TRACKER.signalNewFrame(frame);
  static pose_t current_pose;
  ltr_int_get_camera_update(&(current_pose.yaw), &(current_pose.pitch), &(current_pose.roll), 
                            &(current_pose.tx), &(current_pose.ty), &(current_pose.tz), 
                            &(current_pose.counter));
  TRACKER.signalNewPose(&current_pose);
}

Tracker::Tracker()
{
  master = new MasterThread();
  slave = new SlaveThread();
  ltr_int_set_callback_hooks(ltr_int_new_frame, ltr_int_state_changed);
}

Tracker::~Tracker()
{
  
}

void Tracker::signalStateChange(ltr_state_type current_state)
{
  emit stateChangedz(current_state);
}

void Tracker::signalNewFrame(struct frame_type *frame)
{
  emit newFrame(frame);
}

void Tracker::signalNewPose(pose_t *pose)
{
  emit newPose(pose);
}

void Tracker::setProfile(QString p)
{
  slave->setProfile(p);
}

void Tracker::start()
{
  master->start();
  //slave->start();
  ltr_init((char*)"Default");
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
  ltr_shutdown();
}


