#ifndef TIR_DRIVER__H
#define TIR_DRIVER__H

#include "cal.h"
#include "tir.h"

int tir_init(struct camera_control_block *ccb);
int tir_get_frame(struct camera_control_block *ccb, struct frame_type *f);
int tir_pause();
int tir_resume();
int tir_close();

#endif

