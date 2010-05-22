#ifndef LINUX_TRACK_INT__H
#define LINUX_TRACK_INT__H

#include <stdbool.h>
#include "ltlib.h"

#ifdef __cplusplus
extern "C" {
#endif

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
                         float *tz);
ltr_state_type ltr_int_get_tracking_state(void);

#ifdef __cplusplus
}
#endif

#endif

