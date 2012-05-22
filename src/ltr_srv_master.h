#ifndef LTR_SRV_MASTER__H
#define LTR_SRV_MASTER__H

#include <stdbool.h>
#include <ltlib_int.h>


#ifdef __cplusplus
extern "C" {
#endif

void change(const char *profile, int axis, int elem, float val);
bool master(bool standalone);

//For ltr_gui
void suspend_cmd();
void wakeup_cmd();
void recenter_cmd();
size_t request_shutdown();

void ltr_int_set_callback_hooks(ltr_new_frame_callback_t nfh, ltr_status_update_callback_t suh);
bool ltr_int_gui_lock();

#ifdef __cplusplus
}
#endif
#endif
