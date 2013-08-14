#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "ltlib_int.h"
#include "ipc_utils.h"
#include "utils.h"

static struct mmap_s mmm;
static bool initialized = false;

static int make_mmap()
{
  if(!ltr_int_mmap_file_exclusive(sizeof(struct mmap_s), &mmm)){
    ltr_int_my_perror("mmap_file: ");
    ltr_int_log_message("Couldn't mmap!\n");
    return -1;
  }
  return 0;
}

static void ltr_int_sanitize_name(char *name)
{
  char *forbidden = "\r\n";
  size_t len = strcspn(name, forbidden);
  name[len] = '\0';
}

int ltr_wakeup(void);

char *ltr_int_init_helper(const char *cust_section, bool standalone)
{
  bool is_child;
  if(initialized) return mmm.fname;
  if(make_mmap() != 0) return NULL;
  struct ltr_comm *com = mmm.data;
  com->preparing_start = true;
  initialized = true;
  if(standalone){
    char *server = ltr_int_get_app_path("/ltr_server1");
    if(cust_section == NULL){
      cust_section = "Default";
    }
    char *section = ltr_int_my_strdup(cust_section);
    ltr_int_sanitize_name(section);
    char *args[] = {server, section, mmm.fname, NULL};
    if(!ltr_int_fork_child(args, &is_child)){
      com->state = ERROR;
      free(server);
      if(is_child){
        exit(1);
      }
      return false;
    }
    free(server);
    free(section);
  }
  ltr_wakeup();
  return mmm.fname;
}

int ltr_init(const char *cust_section)
{
  if(ltr_int_init_helper(cust_section, true) != NULL){
    return 0;
  }else{
    return -1;
  }
}

int ltr_get_pose(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz,
                         uint32_t *counter)
{
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)) return -1;
  struct ltr_comm tmp;
  ltr_int_lockSemaphore(mmm.sem);
  tmp = *com;
  //printf("OTHER_SIDE: %g %g %g\n", tmp.pose.yaw, tmp.pose.pitch, tmp.pose.roll);
  ltr_int_unlockSemaphore(mmm.sem);
  if(tmp.state != ERROR){
    *heading = tmp.pose.yaw;
    *pitch = tmp.pose.pitch;
    *roll = tmp.pose.roll;
    *tx = tmp.pose.tx;
    *ty = tmp.pose.ty;
    *tz = tmp.pose.tz;
    *counter = tmp.pose.counter;
    return 0;
  }else{
    return -1;
  }
}

int ltr_get_pose_full(pose_t *pose)
{
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)) return -1;
  struct ltr_comm tmp;
  ltr_int_lockSemaphore(mmm.sem);
  tmp = *com;
  ltr_int_unlockSemaphore(mmm.sem);
  if(tmp.state != ERROR){
    *pose = tmp.pose;
    return 0;
  }else{
    return -1;
  }
}

int ltr_suspend(void)
{
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)) return -1;
  ltr_int_lockSemaphore(mmm.sem);
  com->cmd = PAUSE_CMD;
  ltr_int_unlockSemaphore(mmm.sem);
  return 0;
}

int ltr_wakeup(void)
{
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)) return -1;
  ltr_int_lockSemaphore(mmm.sem);
  com->cmd = RUN_CMD;
  ltr_int_unlockSemaphore(mmm.sem);
  return 0;
}

int ltr_shutdown(void)
{
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)) return -1;
  ltr_int_lockSemaphore(mmm.sem);
  com->cmd = STOP_CMD;
  ltr_int_unlockSemaphore(mmm.sem);
  initialized = false;
  ltr_int_unmap_file(&mmm);
  return 0;
}

void ltr_recenter(void)
{
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)) return;
  ltr_int_lockSemaphore(mmm.sem);
  com->recenter = true;
  ltr_int_unlockSemaphore(mmm.sem);
}

ltr_state_type ltr_get_tracking_state(void)
{
  ltr_state_type state = STOPPED;
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL) || (com->preparing_start)){
    return state;
  }
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

