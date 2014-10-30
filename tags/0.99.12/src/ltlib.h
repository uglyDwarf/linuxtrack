#ifndef LTLIB__H
#define LTLIB__H

#include <stdbool.h>
#include <stdint.h>
#include "linuxtrack.h"

typedef enum{RUN_CMD, PAUSE_CMD, STOP_CMD, NOP_CMD} ltr_cmd;

#define MAX_BLOBS 10
#define BLOB_ELEMENTS 3

typedef struct{
  linuxtrack_pose_t pose;
  uint32_t blobs;
  float blob_list[BLOB_ELEMENTS * MAX_BLOBS];
}linuxtrack_full_pose_t; 

struct ltr_comm{
  uint8_t cmd;
  uint8_t recenter;
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
