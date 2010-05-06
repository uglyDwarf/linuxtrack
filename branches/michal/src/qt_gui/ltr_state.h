#ifndef LTR_STATE__H
#define LTR_STATE__H


#include <QWidget>
#include <QTimer>

#define STATE TrackerState::trackerStateInst()

class TrackerState : public QObject{
  Q_OBJECT
 public:
  static TrackerState& trackerStateInst();
 private:
  TrackerState();
  ~TrackerState();
  static TrackerState *ts;
  QTimer *timer;
 private slots:
  void pollState();
 signals:
  void trackerStopped();
  void trackerPaused();
  void trackerRunning();
};

#endif
