#ifndef LTLIB__H
#define LTLIB__H

#include <stdbool.h>
#include <stdint.h>
#include "linuxtrack.h"

typedef enum{RUN_CMD, PAUSE_CMD, STOP_CMD, NOP_CMD} ltr_cmd;

typedef struct{
  float pitch;
  float yaw;
  float roll;
  float tx;
  float ty;
  float tz;
  uint32_t counter;
  uint8_t status;
} pose_t; 

struct ltr_comm{
  uint8_t cmd;
  uint8_t recenter;
  uint8_t state;
  float heading, pitch, roll;
  float tx, ty, tz;
  uint32_t counter;
  uint8_t dead_man_button;
  uint8_t preparing_start;
};

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIBLINUXTRACK_SRC
int ltr_int_init_helper(const char *cust_section, bool standalone);
#endif

#ifdef __cplusplus
}
#endif

#endif
