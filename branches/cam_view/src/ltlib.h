#ifndef LTLIB__H
#define LTLIB__H

#include <stdbool.h>
#include <stdint.h>
#include "linuxtrack.h"

typedef enum{RUN_CMD, PAUSE_CMD, STOP_CMD, FRAMES_CMD, NOP_CMD} ltr_cmd;

#define MAX_BLOBS 10
#define BLOB_ELEMENTS 3

typedef struct{
  float abs_yaw;
  float abs_pitch;
  float abs_roll;
  float abs_tx;
  float abs_ty;
  float abs_tz;
} linuxtrack_abs_pose_t;

typedef struct{
  linuxtrack_pose_t pose;
  linuxtrack_abs_pose_t abs_pose;
  uint32_t blobs;
  float blob_list[BLOB_ELEMENTS * MAX_BLOBS];
}linuxtrack_full_pose_t;

struct ltr_comm{
  uint8_t cmd;
  uint8_t recenter;
  uint8_t notify;
  int8_t state;
  linuxtrack_full_pose_t full_pose;
  uint8_t dead_man_button;
  uint8_t preparing_start;
};

#ifdef __cplusplus
extern "C" {
#endif

//#ifndef LIBLINUXTRACK_SRC
//char *ltr_int_init_helper(const char *cust_section, bool standalone);
//#endif

#ifdef __cplusplus
}
#endif

#endif
