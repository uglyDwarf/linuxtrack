
#include "ltr_state.h"
#include <ltlib_int.h>

TrackerState *TrackerState::ts = NULL;

TrackerState& TrackerState::trackerStateInst()
{
  if(ts == NULL){
    ts = new TrackerState();
  }
  return *ts;
}

TrackerState::TrackerState()
{
  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(pollState()));
  timer->start(200);
}

TrackerState::~TrackerState()
{
  delete timer;
}

void TrackerState::pollState()
{
  static ltr_state_type last_state = STOPPED;
  ltr_state_type current_state = ltr_int_get_tracking_state();
  if(last_state != current_state){
    switch(current_state){
      case STOPPED:
        emit trackerStopped();
        break;
      case RUNNING:
        emit trackerRunning();
        break;
      case PAUSED:
        emit trackerPaused();
        break;
    }
    last_state = current_state;
  }
}

