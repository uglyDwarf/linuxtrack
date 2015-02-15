#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "ltlib_int.h"
#include "ipc_utils.h"
#include "utils.h"

static struct mmap_s mmm;
static bool initialized = false;
static int notify_pipe = -1;

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
  char pipe0[16];
  char pipe1[16];
  int fd[2];
  bool is_child;
  if(initialized) return mmm.fname;
  if(make_mmap() != 0) return NULL;
  struct ltr_comm *com = mmm.data;
  com->state = INITIALIZING;
  com->preparing_start = true;
  initialized = true;
  if(standalone){
    if(pipe(fd) < 0){
      fd[0] = fd[1] = -1;
    }
    char *server = ltr_int_get_app_path("/ltr_server1");
    if(cust_section == NULL){
      cust_section = "Default";
    }
    char *section = ltr_int_my_strdup(cust_section);
    ltr_int_sanitize_name(section);
    snprintf(pid, sizeof(pid), "%lu", (unsigned long)getpid());
    snprintf(pipe0, sizeof(pipe0), "%d", fd[0]);
    snprintf(pipe1, sizeof(pipe1), "%d", fd[1]);
    char *args[] = {server, section, mmm.fname, pid, pipe0, pipe1, NULL};
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
    close(fd[1]);
    notify_pipe = fd[0];
    printf("Client pipe: %d\n", notify_pipe);
    fcntl(notify_pipe, F_SETFL, fcntl(notify_pipe, F_GETFL) | O_NONBLOCK);
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

linuxtrack_state_type ltr_notification_on(void)
{
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)) return err_NOT_INITIALIZED;
  ltr_int_lockSemaphore(mmm.sem);
  com->notify = true;
  ltr_int_unlockSemaphore(mmm.sem);
  return LINUXTRACK_OK;
}

int ltr_get_notify_pipe(void)
{
  return notify_pipe;
}

linuxtrack_state_type ltr_request_frames(void)
{
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)) return err_NOT_INITIALIZED;
  ltr_int_lockSemaphore(mmm.sem);
  com->cmd = FRAMES_CMD;
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

static struct mmap_s mmap;

int ltr_get_frame(int *req_width, int *req_height, size_t buf_size, uint8_t *buffer)
{
  struct ltr_comm *com = mmm.data;
  if((!initialized) || (com == NULL)) return 0;
  struct ltr_comm tmp;
  ltr_int_lockSemaphore(mmm.sem);
  tmp = *com;
  ltr_int_unlockSemaphore(mmm.sem);
  if(tmp.state < LINUXTRACK_OK){
    return 0;
  }
  uint32_t p_w = tmp.full_pose.pose.resolution_x;
  uint32_t p_h = tmp.full_pose.pose.resolution_y;

  if(p_w * p_h > mmap.size){
    ltr_int_unmap_file(&mmap);
    int data_size = 2 * p_w * p_h + (3 * sizeof(uint32_t));
    char *fname = ltr_int_get_default_file_name("frames.dat");
    if(!ltr_int_mmap_file(fname, data_size, &mmap)){
      free(fname);
      return 0;
    }
    free(fname);
  }
  if(mmap.data == NULL){
    return 0;
  }
  uint32_t *data = (uint32_t*)mmap.data;
  uint32_t *flag = data;
  uint32_t *w = data + 1;
  uint32_t *h = data + 2;
  uint32_t frame_size = (*w) * (*h);
  *req_width = *w;
  *req_height = *h;
  if((p_w * p_h) < frame_size){ //Size had to change, mmap not big enough
    return 0;
  }
  if(buf_size < frame_size){
    return 0;
  }
  uint8_t *buf = ((uint8_t*)mmap.data) + (3 * sizeof(uint32_t)) + (*flag) * frame_size;
  memcpy(buffer, buf, frame_size);
  return 1;
}

int ltr_wait(int timeout)
{
  bool hup = false;
  int res = ltr_int_pipe_poll(notify_pipe, timeout, &hup);
  if(res > 0){
    uint8_t tmp;
    while(read(notify_pipe, &tmp, sizeof(tmp)) > 0);
  }
  return res;
}

