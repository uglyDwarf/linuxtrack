#ifndef TIR_DRIVER__H
#define TIR_DRIVER__H

#include "cal.h"
#include "tir.h"

int ltr_int_tir_found(bool *have_firmware);

int ltr_int_tracker_init(struct camera_control_block *ccb);
int ltr_int_tracker_get_frame(struct camera_control_block *ccb, struct frame_type *f);
int ltr_int_tracker_pause();
int ltr_int_tracker_resume();
int ltr_int_tracker_close();

#endif

