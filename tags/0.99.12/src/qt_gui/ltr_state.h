#ifndef LTR_STATE__H
#define LTR_STATE__H


#include <QWidget>
#include <QTimer>
#include <linuxtrack.h>

#define STATE TrackerState::trackerStateInst()

class TrackerState : public QObject{
  Q_OBJECT
 public:
  static TrackerState& trackerStateInst();
  //linuxtrack_state_type getCurrentState();
 private:
  TrackerState();
  ~TrackerState();
  static TrackerState *ts;
  QTimer *timer;
  linuxtrack_state_type prev_state;
 private slots:
  void pollState();
 signals:
  void stateChanged(linuxtrack_state_type current_state);
};

#endif
