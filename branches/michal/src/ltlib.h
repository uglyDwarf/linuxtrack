#ifndef LINUX_TRACK__H
#define LINUX_TRACK__H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  RUNNING,
  PAUSED,
  STOPPED
}lt_state_type;



int lt_init(char *cust_section);
int lt_shutdown(void);
int lt_suspend(void);
int lt_wakeup(void);
void lt_recenter(void);
int lt_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz);
lt_state_type lt_get_tracking_state(void);
void lt_log_message(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif

