#ifndef WEBCAM_DRIVER__H
#define WEBCAM_DRIVER__H

#include "cal.h"

int webcam_init(struct camera_control_block *ccb);
int webcam_shutdown(struct camera_control_block *ccb);
int webcam_suspend(struct camera_control_block *ccb);
void webcam_change_operating_mode(struct camera_control_block *ccb, 
                             enum cal_operating_mode newmode);
int webcam_wakeup(struct camera_control_block *ccb);
int webcam_get_frame(struct camera_control_block *ccb, struct frame_type *f);




#endif
