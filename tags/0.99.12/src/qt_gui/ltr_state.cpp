
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
  prev_state = STOPPED;
  timer->start(200);
}

TrackerState::~TrackerState()
{
  delete timer;
}

/*
linuxtrack_state_type TrackerState::getCurrentState()
{
  return ltr_int_get_tracking_state();
}
*/

void TrackerState::pollState()
{
  linuxtrack_state_type current_state = ltr_int_get_tracking_state();
  if(prev_state != current_state){
    emit stateChanged(current_state);
  }
  prev_state = current_state;
}

