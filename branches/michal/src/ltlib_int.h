#ifndef LINUX_TRACK_INT__H
#define LINUX_TRACK_INT__H

#include <stdbool.h>
#include "ltlib.h"

#ifdef __cplusplus
extern "C" {
#endif

int lt_int_init(char *cust_section);
int lt_int_shutdown(void);
int lt_int_suspend(void);
int lt_int_wakeup(void);
void lt_int_recenter(void);
int lt_int_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz);
lt_state_type lt_int_get_tracking_state(void);
void lt_int_log_message(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif

