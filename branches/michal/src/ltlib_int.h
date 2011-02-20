#ifndef LINUX_TRACK_INT__H
#define LINUX_TRACK_INT__H

#include <stdbool.h>
#include <stdint.h>
#include "ltlib.h"
#include "linuxtrack.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ltr_callback_t)(void *);

int ltr_int_init(char *cust_section);
int ltr_int_shutdown(void);
int ltr_int_suspend(void);
int ltr_int_wakeup(void);
void ltr_int_recenter(void);
int ltr_int_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz,
                         unsigned int *counter);
ltr_state_type ltr_int_get_tracking_state(void);
void ltr_int_register_cbk(ltr_callback_t new_frame_cbk, void *param1,
                          ltr_callback_t status_change_cbk, void *param2);

#ifdef __cplusplus
}
#endif

#endif

