#ifndef LTLIB__H
#define LTLIB__H

#include <stdbool.h>
#include <stdint.h>
#include "linuxtrack.h"

typedef enum{RUN_CMD, PAUSE_CMD, STOP_CMD, NOP_CMD} ltr_cmd;
struct ltr_comm{
  ltr_cmd cmd;
  bool recenter;
  ltr_state_type state;
  float heading, pitch, roll;
  float tx, ty, tz;
  uint32_t counter;
};


#endif
