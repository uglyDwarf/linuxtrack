#ifndef RUNLOOP__H
#define RUNLOOP__H

int ltr_int_tracker_init(struct camera_control_block *ccb);
int ltr_int_tracker_pause();
int ltr_int_tracker_get_frame(struct camera_control_block *ccb, struct frame_type *frame);
int ltr_int_tracker_resume();
int ltr_int_tracker_close();
#endif
