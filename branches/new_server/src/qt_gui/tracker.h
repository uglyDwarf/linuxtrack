#ifndef TRACKER__H
#define TRACKER__H

#include <QObject>
#include <QThread>
#include <linuxtrack.h>
#include <cal.h>

#define TRACKER Tracker::trackerInst()

class MasterThread;
class SlaveThread;

class Tracker : public QObject{
  Q_OBJECT
 public:
  static Tracker& trackerInst();
  void signalStateChange(ltr_state_type current_state);
  void signalNewFrame(struct frame_type *frame);
  void signalNewPose(pose_t *pose);
  void setProfile(QString p);
 private:
  Tracker();
  ~Tracker();
  static Tracker *trr;
  MasterThread *master;
  SlaveThread *slave;
 public slots:
  void start();
  void pause();
  void wakeup();
  void recenter();
  void stop();
 signals:
  void stateChangedz(int current_state);
  void newFrame(struct frame_type *frame);
  void newPose(pose_t *pose);
};

#endif
