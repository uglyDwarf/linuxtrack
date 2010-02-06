#ifndef TIR_DRIVER__H
#define TIR_DRIVER__H

#include "cal.h"

int ltr_cal_run(struct camera_control_block *ccb, frame_callback_fun cbk);
int ltr_cal_shutdown();
int ltr_cal_suspend();
int ltr_cal_wakeup();
enum cal_device_state_type ltr_cal_get_state();

#endif

