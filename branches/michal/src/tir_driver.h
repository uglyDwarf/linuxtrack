#ifndef TIR_DRIVER__H
#define TIR_DRIVER__H

#include "cal.h"
#include "tir.h"

int tracker_init(struct camera_control_block *ccb);
int tracker_get_frame(struct camera_control_block *ccb, struct frame_type *f);
int tracker_pause();
int tracker_resume();
int tracker_close();

#endif

