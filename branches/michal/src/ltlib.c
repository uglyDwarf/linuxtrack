#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include "cal.h"
#include "ltlib_int.h"
#include "ipc_utils.h"
#include "utils.h"

static int fd;
static char tmp_fname[] = "/tmp/ltrXXXXXX";
static struct mmap_s mmm;

int ltr_init(char *cust_section)
{
  fd = open_tmp_file(tmp_fname);
  if(fd < 0){
    perror("open_tmp_file");
    ltr_int_log_message("Couldn't open communication file!\n");
    return -1;
  }
  if(!mmap_file(tmp_fname, sizeof(struct mmap_s), &mmm)){
    perror("mmap_file: ");
    ltr_int_log_message("Couldn't mmap!\n");
    return -1;
  }
  char *server = ltr_int_get_app_path("/ltr_server");
  char *args[] = {server, tmp_fname, cust_section, NULL};
  fork_child(args);
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
  lockSemaphore(mmm.sem);
  tmp = *com;
  unlockSemaphore(mmm.sem);
  *heading = tmp.heading;
  *pitch = tmp.pitch;
  *roll = tmp.roll;
  *tx = tmp.tx;
  *ty = tmp.ty;
  *tz = tmp.tz;
  *counter = tmp.counter;
  return 0;
}

int ltr_suspend(void)
{
  struct ltr_comm *com = mmm.data;
  lockSemaphore(mmm.sem);
  com->cmd = PAUSE_CMD;
  unlockSemaphore(mmm.sem);
  return 0;
}

int ltr_wakeup(void)
{
  struct ltr_comm *com = mmm.data;
  lockSemaphore(mmm.sem);
  com->cmd = RUN_CMD;
  unlockSemaphore(mmm.sem);
  return 0;
}

int ltr_shutdown(void)
{
  struct ltr_comm *com = mmm.data;
  lockSemaphore(mmm.sem);
  com->cmd = STOP_CMD;
  unlockSemaphore(mmm.sem);
  return 0;
}

void ltr_recenter(void)
{
  struct ltr_comm *com = mmm.data;
  lockSemaphore(mmm.sem);
  com->recenter = true;
  unlockSemaphore(mmm.sem);
}

ltr_state_type ltr_get_tracking_state(void)
{
  ltr_state_type state;
  struct ltr_comm *com = mmm.data;
  lockSemaphore(mmm.sem);
  state = com->state;
  unlockSemaphore(mmm.sem);
  return state;
}

void ltr_log_message(const char *format, ...)
{
  va_list ap;
  va_start(ap,format);
  ltr_int_valog_message(format, ap);
  va_end(ap);
}
