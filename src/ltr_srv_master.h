#ifndef LTR_SRV_MASTER__H
#define LTR_SRV_MASTER__H

#include <stdbool.h>
#include <ltlib_int.h>


#ifdef __cplusplus
extern "C" {
#endif

bool master(bool daemonize);

//For ltr_gui
void ltr_int_set_callback_hooks(ltr_new_frame_callback_t nfh, ltr_status_update_callback_t suh);

#ifdef __cplusplus
}
#endif
#endif
