#include "ltr_srv_comm.h"
#include "ltr_srv_master.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <poll.h>
#include <pthread.h>
#include <unistd.h>
#include <ipc_utils.h>
#include <ltlib.h>
#include <utils.h>
#include <tracking.h>
#include <pref.h>
#include <pref_global.h>

static struct mmap_s mmm;
static int master_uplink = -1;
static int master_downlink = -1;
static pthread_t reader_tid;
static char *profile_name = NULL;
static ltr_axes_t axes;
static bool master_works = false;
static semaphore_p slave_lock = NULL;
static semaphore_p master_lock = NULL;
static const int master_retries = 3;
static pid_t ppid = 0;

typedef enum {MR_OK, MR_FAIL, MR_OFTEN} mr_res_t;

static bool parent_alive()
{
  //Check whether parent lives
  //  (if not, we got orphaned and got adopted by init)
  return (getppid() == ppid);
}

static int ltr_int_open_slave_fifo(int l_master_uplink, const char *name_template, int max_fifos)
{
  ltr_int_log_message("Registering slave fifo @ fd%d...\n", l_master_uplink);
  char *data_fifo_name = NULL;
  //Open the data passing fifo and pass it to the master...
  int fifo_number = -1;
  int l_master_downlink = ltr_int_open_unique_fifo(&data_fifo_name, &fifo_number, name_template,
                                                 max_fifos, &slave_lock);
  ltr_int_log_message("Trying to open unique fifo %s => %d\n", data_fifo_name, l_master_downlink);
  free(data_fifo_name);
  if(l_master_downlink <= 0){
    ltr_int_log_message("Failed to open master downlink!\n");
    return -1;
  }
  if(ltr_int_send_message_w_str(l_master_uplink, CMD_NEW_FIFO, fifo_number, profile_name) == 0){
    ltr_int_log_message("Master downlink (%d) OK!\n", l_master_downlink);
    return l_master_downlink;
  }else{
    ltr_int_log_message("Master uplink not working!\n");
    close(l_master_downlink);
    return -1;
  }
}

static bool close_master_comms(int *l_master_uplink, int *l_master_downlink)
{
  if(*l_master_downlink > 0){
    close(*l_master_downlink);
    *l_master_downlink = -1;
  }
  if(*l_master_uplink > 0){
    close(*l_master_uplink);
    *l_master_uplink = -1;
  }
  return true;
}


static bool start_master(int *l_master_uplink, int *l_master_downlink)
{
  bool is_child;
  //master inherits fds!
  int fifo = ltr_int_open_fifo_exclusive(ltr_int_master_fifo_name(), &master_lock);
  if(fifo > 0){
    //no master there yet, so lets start one
    close(fifo);
    ltr_int_unlockSemaphore(master_lock);
    ltr_int_closeSemaphore(master_lock);
    close_master_comms(l_master_uplink, l_master_downlink);
    ltr_int_log_message("Master is not running, start it\n");
    char *args[] = {"srv", NULL};
    args[0] = ltr_int_get_app_path("/ltr_server1");
    ltr_int_fork_child(args, &is_child);
    int status;
    //Disable the wait when not daemonizing master!!!
    wait(&status);
    //At this point master is either running or exited (depending on the state of fifo)
    free(args[0]);
  }
  //At this point either master runs already, or we just started one
  return true;
}

static bool open_master_comms(int *l_master_uplink, int *l_master_downlink)
{
  ltr_int_log_message("Opening master comms!\n");
  *l_master_uplink = ltr_int_open_fifo_for_writing(ltr_int_master_fifo_name(), true);
  if(*l_master_uplink <= 0){
    ltr_int_log_message("Couldn't open fifo to master!\n");
    return false;
  }
  if((*l_master_downlink = ltr_int_open_slave_fifo(*l_master_uplink, ltr_int_slave_fifo_name(),
                                               ltr_int_max_slave_fifos())) <= 0){
    ltr_int_log_message("Couldn't pass master our fifo!\n");
    close(*l_master_uplink);
    *l_master_uplink = -1;
    *l_master_downlink = -1;
    return false;
  }
  ltr_int_log_message("Master comms opened => u -> %d  d -> %d\n", *l_master_uplink, *l_master_downlink);
  return true;
}

static bool ltr_int_try_start_master(int *l_master_uplink, int *l_master_downlink)
{
  int master_restart_retries = master_retries;
  while(master_restart_retries > 0){

    close_master_comms(l_master_uplink, l_master_downlink);
    if(ltr_int_gui_lock(false)){

      start_master(l_master_uplink, l_master_downlink);
      --master_restart_retries;
    }else{
      master_restart_retries = master_retries;
    }

    if(open_master_comms(l_master_uplink, l_master_downlink)){
      ltr_int_log_message("Master is responding!\n");
      return true;
    }
    sleep(2);
  }
  return false;
}


static bool ltr_int_process_message(int l_master_downlink)
{
  message_t msg;
  struct ltr_comm *com;
  linuxtrack_pose_t unfiltered;
  ssize_t bytesRead = ltr_int_fifo_receive(l_master_downlink, &msg, sizeof(message_t));
  if(bytesRead < 0){
    ltr_int_log_message("Slave reader problem!\n");
    ltr_int_my_perror("fifo_receive");
    return false;
  }else if(bytesRead == 0){
    return true;
  }

  switch(msg.cmd){
    case CMD_NOP:
      break;
    case CMD_POSE:
      //printf("Have new pose!\n");
      //printf(">>>>%f %f %f\n", msg.pose.raw_yaw, msg.pose.raw_pitch, msg.pose.raw_tz);
      ltr_int_postprocess_axes(axes, &(msg.pose.pose), &unfiltered);
      //printf(">>>>%f %f %f\n", msg.pose.yaw, msg.pose.pitch, msg.pose.tz);
      //printf("Raw center: %f  %f  %f\n", msg.pose.pose.raw_tx, msg.pose.pose.raw_ty, msg.pose.pose.raw_tz);
      //printf("Raw angles: %f  %f  %f\n", msg.pose.pose.raw_pitch, msg.pose.pose.raw_yaw, msg.pose.pose.raw_roll);

      com = mmm.data;
      ltr_int_lockSemaphore(mmm.sem);
      //printf("STATUS: %d\n", msg.pose.status);
      if(msg.pose.pose.status == RUNNING){
        //printf("PASSING TO SHM: %f %f %f\n", msg.pose.yaw, msg.pose.pitch, msg.pose.tz);
        com->full_pose = msg.pose;
      }
      com->state = msg.pose.pose.status;
      com->preparing_start = false;
      ltr_int_unlockSemaphore(mmm.sem);
      break;
    case CMD_PARAM:
      //printf("Changing %s of %s to %f!!!\n", ltr_int_axis_param_get_desc(msg.param.param_id),
      //  ltr_int_axis_get_desc(msg.param.axis_id), msg.param.flt_val);
      if(msg.param.axis_id == MISC){
        switch(msg.param.param_id){
          case MISC_ALTER:
            ltr_int_set_use_alter(msg.param.flt_val > 0.5f);
            break;
          case MISC_ALIGN:
            ltr_int_set_tr_align(msg.param.flt_val > 0.5f);
            break;
          case MISC_LEGR:
            ltr_int_set_use_oldrot(msg.param.flt_val > 0.5f);
            break;
          case MISC_FOCAL_LENGTH:
            ltr_int_set_focal_length(msg.param.flt_val);
            break;
          default:
            ltr_int_log_message("Wrong misc param: %d\n", msg.param.param_id);
            return false;
            break;
        }
      }else if(msg.param.param_id == AXIS_ENABLED){
        ltr_int_set_axis_bool_param(axes, msg.param.axis_id, msg.param.param_id, msg.param.flt_val > 0.5f);
      }else{
        ltr_int_set_axis_param(axes, msg.param.axis_id, msg.param.param_id, msg.param.flt_val);
      }
      break;
    default:
      ltr_int_log_message("Slave received unexpected message %d!\n", msg.cmd);
      return false;
      break;
  }
  //printf("Received: '%s'\n", msg.str);
  return true;
}


static void *ltr_int_slave_reader_thread(void *param)
{
  ltr_int_log_message("Slave reader thread function entered!\n");
  (void) param;
  int received_frames = 0;
  master_works = false;
  while(1){
    if(!ltr_int_try_start_master(&master_uplink, &master_downlink)){
      break;
    }
    ltr_int_log_message("Master Uplink %d, Downlink %d\n", master_uplink, master_downlink);
    int poll_errs = 0;
    struct pollfd downlink_poll= {
      .fd = master_downlink,
      .events = POLLIN,
      .revents = 0
    };
    while(1){
      downlink_poll.events = POLLIN;
      int fds = poll(&downlink_poll, 1, 1000);
      if(fds < 0){
        ++poll_errs;
        ltr_int_my_perror("poll");
        if(poll_errs > 3){break;}else{continue;}
      }else if(fds == 0){
        if(!parent_alive()){
          //printf("Parent %lu died! (1)\n", (unsigned long)ppid);
          return NULL;
        }
        continue;
      }
      if(downlink_poll.revents & POLLHUP){
        break;
      }
      //We have a new message
      if(ltr_int_process_message(master_downlink)){
        ++received_frames;
        if(received_frames > 20){
          master_works = true;
        }
      }
      if(!parent_alive()){
        //printf("Parent %lu died! (2)\n", (unsigned long)ppid);
        return NULL;
      }
    }
  }
  return NULL;
}


static void ltr_int_slave_main_loop()
{
  //Prepare to process client requests
  struct ltr_comm *com = mmm.data;
  ltr_cmd cmd = NOP_CMD;
  bool recenter = false;
  bool quit_flag = false;
  while(!quit_flag){
    if((com->cmd != NOP_CMD) || com->recenter){
      ltr_int_lockSemaphore(mmm.sem);
      cmd = (ltr_cmd)com->cmd;
      com->cmd = NOP_CMD;
      recenter = com->recenter;
      com->recenter = false;
      ltr_int_unlockSemaphore(mmm.sem);
    }
    switch(cmd){
      case PAUSE_CMD:
        ltr_int_send_message(master_uplink, CMD_PAUSE, 0);
        break;
      case RUN_CMD:
        ltr_int_send_message(master_uplink, CMD_WAKEUP, 0);
        break;
      case STOP_CMD:
        quit_flag = true;
        break;
      default:
        break;
    }
    cmd = NOP_CMD;
    if(recenter){
      ltr_int_log_message("Slave sending master recenter request!\n");
      ltr_int_send_message(master_uplink, CMD_RECENTER, 0);
    }
    recenter = false;
    if(!parent_alive()){
      //printf("Parent %lu died! (3)\n", (unsigned long)ppid);
      break;
    }
    usleep(100000);
  }
}

//main slave function

bool ltr_int_slave(const char *c_profile, const char *c_com_file, const char *ppid_str)
{
  unsigned long tmp_ppid;
  profile_name = ltr_int_my_strdup(c_profile);
  sscanf(ppid_str, "%lu", &tmp_ppid);
  //printf("Going to monitor parent %lu!\n", tmp_ppid);
  ppid = (pid_t)tmp_ppid;
  if(!ltr_int_read_prefs(NULL, false)){
    ltr_int_log_message("Couldn't load preferences!\n");
    return false;
  }
  ltr_int_init_axes(&axes, profile_name);
  //Prepare client comm channel
  char *com_file = ltr_int_my_strdup(c_com_file);
  if(!ltr_int_mmap_file(com_file, sizeof(struct ltr_comm), &mmm)){
    ltr_int_log_message("Couldn't mmap file!!!\n");
    return false;
  }
  free(com_file);

  if(pthread_create(&reader_tid, NULL, ltr_int_slave_reader_thread, NULL) == 0){
    ltr_int_slave_main_loop();
    pthread_join(reader_tid, NULL);
  }
  close_master_comms(&master_uplink, &master_downlink);
  ltr_int_unmap_file(&mmm);
  //finish prefs
  ltr_int_close_axes(&axes);
  ltr_int_free_prefs();
  free(profile_name);
  ltr_int_gui_lock_clean();
  return true;
}


