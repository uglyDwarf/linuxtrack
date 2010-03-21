#ifndef WEBCAM_DRIVER__H
#define WEBCAM_DRIVER__H

#include <linux/types.h>
#include "cal.h"

#ifdef __cplusplus
extern "C" {
#endif

int webcam_init(struct camera_control_block *ccb);
int webcam_shutdown();
int webcam_suspend();
int webcam_change_operating_mode(struct camera_control_block *ccb, 
                             enum cal_operating_mode newmode);
int webcam_wakeup();
int webcam_get_frame(struct camera_control_block *ccb, struct frame_type *f);

extern dev_interface webcam_interface;


typedef struct{
  int i; //index into pixel format table
  __u32 fourcc;
  int w;
  int h;
  int fps_num, fps_den; 
} webcam_format;

typedef struct{
  char **fmt_strings; //Format table
  webcam_format *formats;
  int entries;
} webcam_formats;

int enum_webcams(char **ids[]);
int enum_webcam_formats(char *id, webcam_formats *formats);
int enum_webcam_formats_cleanup(webcam_formats *all_formats);

#ifdef __cplusplus
}
#endif

#endif
