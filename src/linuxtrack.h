#ifndef LINUX_TRACK__H
#define LINUX_TRACK__H

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

int linuxtrack_shutdown(void);
int linuxtrack_suspend(void);
int linuxtrack_wakeup(void);
int linuxtrack_recenter(void);
int linuxtrack_get_pose(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz,
                         uint32_t *counter);
ltr_state_type linuxtrack_get_tracking_state(void);
int linuxtrack_init(const char *cust_section);

#ifdef __cplusplus
}
#endif

#endif

