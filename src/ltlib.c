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

static char *ltr_int_init_helper(const char *cust_section, bool standalone)
{
  char pid[16];
  bool is_child;
  if(initialized) return mmm.fname;
  if(make_mmap() != 0) return NULL;
  struct ltr_comm *com = mmm.data;
  com->state = INITIALIZING;
  com->preparing_start = true;
  initialized = true;
  if(standalone){
    char *server = ltr_int_get_app_path("/ltr_server1");
    if(cust_section == NULL){
      cust_section = "Default";
    }
    char *section = ltr_int_my_strdup(cust_section);
    ltr_int_sanitize_name(section);
    snprintf(pid, sizeof(pid), "%lu", (unsigned long)getpid());
    char *args[] = {server, section, mmm.fname, pid, NULL};
    if(!ltr_int_fork_child(args, &is_child)){
      com->state = err_NOT_INITIALIZED;
      free(server);
      if(is_child){
        exit(1);
      }
      return NULL;
    }
    free(server);
    free(section);
  }
  ltr_wakeup();
  return mmm.fname;
}

linuxtrack_state_type ltr_get_tracking_state(void);

linuxtrack_state_type ltr_init(const char *cust_section)
{
  if(ltr_int_init_helper(cust_section, true) != NULL){
    return ltr_get_tracking_state();
  }else{
    return INITIALIZING;
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
  if((!initialized) || (com == NULL)) return 0;
  struct ltr_comm tmp;
  ltr_int_lockSemaphore(mmm.sem);
  tmp = *com;
  //printf("OTHER_SIDE: %g %g %g\n", tmp.pose.yaw, tmp.pose.pitch, tmp.pose.roll);
  ltr_int_unlockSemaphore(mmm.sem);
  if(tmp.state >= LINUXTRACK_OK){
    uint32_t passed_counter = *counter;
    *heading = tmp.full_pose.pose.yaw;
    *pitch = tmp.full_pose.pose.pitch;
    *roll = tmp.full_pose.pose.roll;
    *tx = tmp.full_pose.pose.tx;
    *ty = tmp.full_pose.pose.ty;
    *tz = tmp.full_pose.pose.tz;
    *counter = tmp.full_pose.pose.counter;
    if(passed_counter != *counter){
      return 1;// flag new data
    }else{
      return 0;
    }
  }else{
    *heading = 0.0;
    *pitch = 0.0;
    *roll = 0.0;
    *tx = 0.0;
    *ty = 0.0;
    *tz = 0.0;
    *counter = 0;
    return 0;
  }
}

int ltr_get_pose_full(linuxtrack_pose_t *pose, float blobs[], int num_blobs, int *blobs_read)
{
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)) return 0;
  struct ltr_comm tmp;
  ltr_int_lockSemaphore(mmm.sem);
  tmp = *com;
  ltr_int_unlockSemaphore(mmm.sem);
  if(tmp.state >= LINUXTRACK_OK){
    uint32_t prev_counter = pose->counter;
    *pose = tmp.full_pose.pose;
    *blobs_read = (num_blobs < (int)tmp.full_pose.blobs) ? num_blobs : (int)tmp.full_pose.blobs;
    int i;
    for(i = 0; i < (*blobs_read) * BLOB_ELEMENTS; ++i){
      blobs[i] = tmp.full_pose.blob_list[i];
    }
    if(prev_counter != pose->counter){
      return 1;//new data
    }else{
      return 0;
    }
  }else{
    *blobs_read = 0;
    memset(pose, 0, sizeof(linuxtrack_pose_t));
    return 0;
  }
}

int ltr_get_abs_pose(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz,
                         uint32_t *counter)
{
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)) return 0;
  struct ltr_comm tmp;
  ltr_int_lockSemaphore(mmm.sem);
  tmp = *com;
  //printf("OTHER_SIDE: %g %g %g\n", tmp.pose.yaw, tmp.pose.pitch, tmp.pose.roll);
  ltr_int_unlockSemaphore(mmm.sem);
  if(tmp.state >= LINUXTRACK_OK){
    uint32_t passed_counter = *counter;
    *heading = tmp.full_pose.abs_pose.abs_yaw;
    *pitch = tmp.full_pose.abs_pose.abs_pitch;
    *roll = tmp.full_pose.abs_pose.abs_roll;
    *tx = tmp.full_pose.abs_pose.abs_tx;
    *ty = tmp.full_pose.abs_pose.abs_ty;
    *tz = tmp.full_pose.abs_pose.abs_tz;
    *counter = tmp.full_pose.pose.counter;
    if(passed_counter != *counter){
      return 1;// flag new data
    }else{
      return 0;
    }
  }else{
    *heading = 0.0;
    *pitch = 0.0;
    *roll = 0.0;
    *tx = 0.0;
    *ty = 0.0;
    *tz = 0.0;
    *counter = 0;
    return 0;
  }
}


linuxtrack_state_type ltr_suspend(void)
{
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)) return err_NOT_INITIALIZED;
  ltr_int_lockSemaphore(mmm.sem);
  com->cmd = PAUSE_CMD;
  ltr_int_unlockSemaphore(mmm.sem);
  return LINUXTRACK_OK;
}

linuxtrack_state_type ltr_wakeup(void)
{
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)) return err_NOT_INITIALIZED;
  ltr_int_lockSemaphore(mmm.sem);
  com->cmd = RUN_CMD;
  ltr_int_unlockSemaphore(mmm.sem);
  return LINUXTRACK_OK;
}

linuxtrack_state_type ltr_shutdown(void)
{
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)) return err_NOT_INITIALIZED;
  ltr_int_lockSemaphore(mmm.sem);
  com->cmd = STOP_CMD;
  ltr_int_unlockSemaphore(mmm.sem);
  initialized = false;
  ltr_int_unmap_file(&mmm);
  return LINUXTRACK_OK;
}

linuxtrack_state_type ltr_recenter(void)
{
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)) return err_NOT_INITIALIZED;
  ltr_int_lockSemaphore(mmm.sem);
  com->recenter = true;
  ltr_int_unlockSemaphore(mmm.sem);
  return LINUXTRACK_OK;
}

linuxtrack_state_type ltr_get_tracking_state(void)
{
  linuxtrack_state_type state = STOPPED;
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)){
    return err_NOT_INITIALIZED;
  }
  if(com->preparing_start){
    return INITIALIZING;
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

const char *ltr_explain(linuxtrack_state_type status)
{
  const char *res = NULL;
  switch(status){
    case INITIALIZING:
      res = "Linuxtrack is initializing.";
      break;
    case RUNNING:
      res = "Linuxtrack is running.";
      break;
    case PAUSED:
      res = "Linuxtrack is paused.";
      break;
    case STOPPED:
      res = "Linuxtrack is stopped.";
      break;
    case err_NOT_INITIALIZED:
      res = "Linuxtrack function was called without proper initialization.";
      break;
    case err_SYMBOL_LOOKUP:
      res = "Internal error (symbol lookup). Please file an issue at Linuxtrack project page.";
      break;
    case err_NO_CONFIG:
      res = "Linuxtrack config not found. If you have Linuxtrack, run ltr_gui and set it up first.";
      break;
    case err_NOT_FOUND:
      res = "Linuxtrack was removed or relocated. If you relocated it,\n"
            "run ltr_gui from the new location, save preferences and try again.";
      break;
    case err_PROCESSING_FRAME:
      res = "Internal error (frame processing). Please file an issue at Linuxtrack project page.";
      break;
    default:
      printf("UNKNOWN status code. Please file an issue at Linuxtrack project page.\n");
      break;
  }
  return res;
}
