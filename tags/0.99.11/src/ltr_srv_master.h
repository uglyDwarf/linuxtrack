#ifndef LTR_SRV_MASTER__H
#define LTR_SRV_MASTER__H

#include <stdbool.h>
#include <ltlib_int.h>


#ifdef __cplusplus
extern "C" {
#endif

void ltr_int_change(const char *profile, int axis, int elem, float val);
bool ltr_int_master(bool standalone);

//For ltr_gui
void ltr_int_suspend_cmd();
void ltr_int_wakeup_cmd();
void ltr_int_recenter_cmd();
size_t ltr_int_request_shutdown();

void ltr_int_set_callback_hooks(ltr_new_frame_callback_t nfh, ltr_status_update_callback_t suh, 
                                ltr_new_slave_callback_t nsh);
bool ltr_int_gui_lock(bool do_lock);
void ltr_int_gui_lock_clean();

#ifdef __cplusplus
}
#endif
#endif
