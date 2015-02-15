#ifndef TRACKER__H
#define TRACKER__H

#include <QObject>
#include <QThread>
#include <linuxtrack.h>
#include <axis.h>
#include <cal.h>
#include "buffering.h"

#define TRACKER Tracker::trackerInst()

class MasterThread;

class Tracker : public QObject{
  Q_OBJECT
 public:
  static Tracker& trackerInst();
  //Housekeeping functions (callbacks)
  void signalStateChange(linuxtrack_state_type current_state);
  void signalNewFrame(struct frame_type *frame);
  void signalNewPose(linuxtrack_full_pose_t *ful_pose);
  void signalNewSlave(const char *name);
  //Profile related stuff
  void setProfile(QString p);

  bool axisChange(axis_t axis, axis_param_t elem, bool enabled);
  bool axisChange(axis_t axis, axis_param_t elem, float val);
  bool miscChange(axis_param_t elem, bool enabled);
  bool miscChange(axis_param_t elem, float val);

  bool axisGetBool(axis_t axis, axis_param_t elem);
  float axisGet(axis_t axis, axis_param_t elem);

  float axisGetValue(axis_t axis, float val);

  bool axisIsSymetrical(axis_t axis);
  bool setCommonFilterFactor(float c_f);
  float getCommonFilterFactor();
  void fromDefault();
  static buffering *getBuffers();
 private:
  Tracker();
  ~Tracker();
  static Tracker *trr;
  MasterThread *master;
  ltr_axes_t axes;
  bool axes_valid;
  QString currentProfile;
  QString profileSection;
  float common_ff;
  float ffs[6];
 public slots:
  void start(QString &section);
  void pause();
  void wakeup();
  void recenter();
  void stop();
 signals:
  void stateChanged(int current_state);
  void newFrame(struct frame_type *frame);
  void newPose(linuxtrack_full_pose_t *raw_pose, linuxtrack_pose_t *unfiltered, linuxtrack_pose_t *pose);
  void axisChanged(int axis, int elem);
  void setCommonFF(float val);
  void initAxes();
  void quitRequest();
};

#endif

