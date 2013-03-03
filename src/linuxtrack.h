#ifndef LINUX_TRACK__H
#define LINUX_TRACK__H

#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  INITIALIZING,
  RUNNING,
  PAUSED,
  STOPPED,
  ERROR
}ltr_state_type;



int ltr_init(const char *cust_section);
int ltr_shutdown(void);
int ltr_suspend(void);
int ltr_wakeup(void);
void ltr_recenter(void);
int ltr_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz,
                         uint32_t *counter);
ltr_state_type ltr_get_tracking_state(void);
void ltr_log_message(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif

