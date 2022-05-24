#include "ltr_srv_comm.h"
#include "ltr_srv_master.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <poll.h>
#include <fcntl.h>
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
static pthread_t reader_tid;
static char *profile_name = NULL;
static ltr_axes_t axes;
static bool master_works = false;
static const int master_retries = 3;
static pid_t ppid = 0;
static int notify_pipe = -1;
static bool notify = false;

typedef enum {MR_OK, MR_FAIL, MR_OFTEN} mr_res_t;

static bool parent_alive()
{
  //Check whether parent lives
  //  (if not, we got orphaned and got adopted by init)
  return (getppid() == ppid);
}

static bool close_master_comms(int *l_master_uplink)
{
  if(*l_master_uplink > 0){
    ltr_int_close_socket(*l_master_uplink);
    *l_master_uplink = -1;
  }
  return true;
}


static bool start_master()
{
  bool is_child;
  //master inherits fds!
  ltr_int_log_message("Start master\n");
  int socket = ltr_int_connect_to_socket(ltr_int_master_socket_name());

  ltr_int_log_message("socket %d\n", socket);
  if(socket < 0){
    //no master there yet, so lets start one
    ltr_int_log_message("Master is not running, start it\n");

    if(!getenv("LTR_KEEP_LD_LIBRARY_PATH")){
      ltr_int_log_message("Reseting LD_LIBRARY_PATH.\n");
      unsetenv("LD_LIBRARY_PATH");
    }else{
      ltr_int_log_message("LTR_KEEP_LD_LIBRARY_PATH set, keeping LD_LIBRARY_PATH set.\n");
    }

    char *args[] = {"srv", NULL};
    args[0] = ltr_int_get_app_path("/ltr_server1");
    ltr_int_fork_child(args, &is_child);
    int status;
    //Disable the wait when not daemonizing master!!!
    wait(&status);
    //At this point master is either running or exited (depending on the state of socket)
    free(args[0]);
  }
  close(socket);
  //At this point either master runs already, or we just started one
  return true;
}

static bool open_master_comms(int *l_master_uplink)
{
  ltr_int_log_message("Opening master comms!\n");
  //printf("open_master_comms\n");
  *l_master_uplink = ltr_int_connect_to_socket(ltr_int_master_socket_name());
  //printf("====================");
  if(*l_master_uplink <= 0){
    ltr_int_log_message("Couldn't connect to master's socket!\n");
    return false;
  }

  ltr_int_log_message("Master comms opened => u -> %d\n", *l_master_uplink);

  if(ltr_int_send_message_w_str(*l_master_uplink, CMD_NEW_SOCKET, 0, profile_name) != 0){
    ltr_int_log_message("Master uplink doesn't seem to be working!\n");
    return false;
  }
  ltr_int_log_message("Notification of the '%s' client sent to master!\n", profile_name);

  return true;
}

static bool ltr_int_try_start_master(int *l_master_uplink)
{
  int master_restart_retries = master_retries;
  while(master_restart_retries > 0){
    close_master_comms(l_master_uplink);
    if(ltr_int_gui_lock(false)){
      ltr_int_log_message("GUI lock retrieved, no GUI running.\n");
      start_master();
      usleep(100);
      --master_restart_retries;
    }else{
      master_restart_retries = master_retries;
    }

    if(open_master_comms(l_master_uplink)){
      ltr_int_log_message("Master is responding!\n");
      return true;
    }
    sleep(1);
  }
  return false;
}

static linuxtrack_pose_t prev_filtered_pose;

static bool ltr_int_process_message(int l_master_uplink)
{
  message_t msg;
  struct ltr_comm *com;
  linuxtrack_pose_t unfiltered;
  ssize_t bytesRead = ltr_int_socket_receive(l_master_uplink, &msg, sizeof(message_t));
  if(bytesRead < 0){
    ltr_int_log_message("Slave reader problem!\n");
    ltr_int_my_perror("socket_receive");
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
        com->full_pose.prev_pose = prev_filtered_pose;
        prev_filtered_pose = msg.pose.pose;
      }
      com->state = msg.pose.pose.status;
      com->preparing_start = false;
      ltr_int_unlockSemaphore(mmm.sem);
      if(notify && (notify_pipe > 0)){
        uint8_t tmp = 0;
        if(write(notify_pipe, &tmp, 1) < 0){
          //Don't report, it would overfill logs
        }
      }
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

static bool quit_flag;

static void *ltr_int_slave_reader_thread(void *param)
{
  ltr_int_log_message("Slave reader thread function entered!\n");
  (void) param;
  int received_frames = 0;
  master_works = false;
  while(1){
    if(!ltr_int_try_start_master(&master_uplink)){
      break;
    }
    ltr_int_log_message("Master Uplink %d\n", master_uplink);
    int poll_errs = 0;
    struct pollfd uplink_poll= {
      .fd = master_uplink,
      .events = POLLIN,
      .revents = 0
    };
    while(1){
      uplink_poll.events = POLLIN;
      uplink_poll.revents = 0;
      int fds = poll(&uplink_poll, 1, 1000);
      if(fds < 0){
        ++poll_errs;
        ltr_int_my_perror("poll");
        if(poll_errs > 3){break;}else{continue;}
      }else if(fds == 0){
        if(!parent_alive() || quit_flag){
          //printf("Parent %lu died! (1)\n", (unsigned long)ppid);
          return NULL;
        }
        continue;
      }
      if(uplink_poll.revents & POLLHUP){
        ltr_int_log_message("Got hangup from master uplink.\n");
        break;
      }
      //We have a new message
      if(ltr_int_process_message(master_uplink)){
        ++received_frames;
        if(received_frames > 20){
          master_works = true;
        }
      }
      if(!parent_alive() || quit_flag){
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
  while(!quit_flag){
    if((com->cmd != NOP_CMD) || com->recenter || com->notify){
      ltr_int_lockSemaphore(mmm.sem);
      cmd = (ltr_cmd)com->cmd;
      com->cmd = NOP_CMD;
      recenter = com->recenter;
      notify = com->notify;
      com->recenter = false;
      ltr_int_unlockSemaphore(mmm.sem);
    }
    int res = 0;
    do{
      switch(cmd){
        case PAUSE_CMD:
          ltr_int_log_message("Sending pause command to master @ fd %d\n", master_uplink);
          res = ltr_int_send_message(master_uplink, CMD_PAUSE, 0);
          break;
        case RUN_CMD:
          ltr_int_log_message("Sending run command to master @ fd %d\n", master_uplink);
          res = ltr_int_send_message(master_uplink, CMD_WAKEUP, 0);
          break;
        case STOP_CMD:
          quit_flag = true;
          break;
        case FRAMES_CMD:
          ltr_int_log_message("Sending frames command to master @ fd %d\n", master_uplink);
          res = ltr_int_send_message(master_uplink, CMD_FRAMES, 0);
          break;
        default:
          break;
      }
      if(res < 0){
        usleep(100000);
      }
      if(!parent_alive()){
        //printf("Parent %lu died! (3)\n", (unsigned long)ppid);
        break;
      }
    }while((res < 0) && (!quit_flag));
    cmd = NOP_CMD;
    if(recenter){
      ltr_int_log_message("Slave sending master recenter request!\n");
      if(ltr_int_send_message(master_uplink, CMD_RECENTER, 0) >= 0){
        //clear request only on successfull transmission
        recenter = false;
      }
    }
    if(!parent_alive()){
      //printf("Parent %lu died! (3)\n", (unsigned long)ppid);
      break;
    }
    usleep(100000);
  }
}

//main slave function

bool ltr_int_slave(const char *c_profile, const char *c_com_file, const char *ppid_str,
                   const char *close_pipe_str, const char *notify_pipe_str)
{
  unsigned long tmp_ppid;
  profile_name = ltr_int_my_strdup(c_profile);
  sscanf(ppid_str, "%lu", &tmp_ppid);
  int tmp_pipe = -1;
  sscanf(close_pipe_str, "%d", &tmp_pipe);
  if(tmp_pipe > 0){
    close(tmp_pipe);
  }
  sscanf(notify_pipe_str, "%d", &notify_pipe);
  if(notify_pipe > 0){
    fcntl(notify_pipe, F_SETFL, fcntl(notify_pipe, F_GETFL) | O_NONBLOCK);
  }
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

  quit_flag = false;
  if(pthread_create(&reader_tid, NULL, ltr_int_slave_reader_thread, NULL) == 0){
    ltr_int_slave_main_loop();
    pthread_join(reader_tid, NULL);
  }
  close_master_comms(&master_uplink);
  ltr_int_unmap_file(&mmm);
  //finish prefs
  ltr_int_close_axes(&axes);
  ltr_int_free_prefs();
  free(profile_name);
  ltr_int_gui_lock_clean();
  close(notify_pipe);
  return true;
}


