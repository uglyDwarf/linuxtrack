#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include "ltlib_int.h"

#ifndef LIBLINUXTRACK_SRC
  #include "ipc_utils.h"
  #include "utils.h"
#endif

static char *com_fname;
static struct mmap_s mmm;

static int make_mmap()
{
  com_fname = ltr_int_get_com_file_name();
  if(!ltr_int_mmap_file(com_fname, sizeof(struct mmap_s), &mmm)){
    perror("mmap_file: ");
    ltr_int_log_message("Couldn't mmap!\n");
    return -1;
  }
  return 0;
}

int ltr_init(char *cust_section)
{
  if(make_mmap() != 0) return -1;
  char *server = ltr_int_get_app_path("/ltr_server");
  char *args[] = {server, cust_section, NULL};
  ltr_int_fork_child(args);
  free(server);
  return 0;
}

int ltr_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz,
                         unsigned int *counter)
{
  struct ltr_comm *com = mmm.data;
  struct ltr_comm tmp;
  ltr_int_lockSemaphore(mmm.sem);
  tmp = *com;
  ltr_int_unlockSemaphore(mmm.sem);
  if(tmp.state != DOWN){
    *heading = tmp.heading;
    *pitch = tmp.pitch;
    *roll = tmp.roll;
    *tx = tmp.tx;
    *ty = tmp.ty;
    *tz = tmp.tz;
    *counter = tmp.counter;
    return 0;
  }else{
    return -1;
  }
}

int ltr_suspend(void)
{
  struct ltr_comm *com = mmm.data;
  ltr_int_lockSemaphore(mmm.sem);
  com->cmd = PAUSE_CMD;
  ltr_int_unlockSemaphore(mmm.sem);
  return 0;
}

int ltr_wakeup(void)
{
  struct ltr_comm *com = mmm.data;
  ltr_int_lockSemaphore(mmm.sem);
  com->cmd = RUN_CMD;
  ltr_int_unlockSemaphore(mmm.sem);
  return 0;
}

int ltr_shutdown(void)
{
  struct ltr_comm *com = mmm.data;
  ltr_int_lockSemaphore(mmm.sem);
  com->cmd = STOP_CMD;
  ltr_int_unlockSemaphore(mmm.sem);
  return 0;
}

void ltr_recenter(void)
{
  struct ltr_comm *com = mmm.data;
  ltr_int_lockSemaphore(mmm.sem);
  com->recenter = true;
  ltr_int_unlockSemaphore(mmm.sem);
}

ltr_state_type ltr_get_tracking_state(void)
{
  ltr_state_type state = DOWN;
  struct ltr_comm *com = mmm.data;
  ltr_int_lockSemaphore(mmm.sem);
  state = com->state;
  ltr_int_unlockSemaphore(mmm.sem);
  return state;
}

void ltr_log_message(const char *format, ...)
{
  va_list ap;
  va_start(ap,format);
  ltr_int_valog_message(format, ap);
  va_end(ap);
}
