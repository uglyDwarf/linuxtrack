#ifndef TIR_DRIVER__H
#define TIR_DRIVER__H

#include "cal.h"

int tir_init(struct camera_control_block *ccb);
int tir_shutdown(struct camera_control_block *ccb);
int tir_suspend(struct camera_control_block *ccb);
int tir_change_operating_mode(struct camera_control_block *ccb,
                             enum cal_operating_mode newmode);
int tir_wakeup(struct camera_control_block *ccb);
int tir_get_frame(struct camera_control_block *ccb, struct frame_type *f);


extern dev_interface tir_interface;



#endif

