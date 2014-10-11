#include <stdio.h>
#include <stdlib.h>
#include "ltr_srv_comm.h"
#include "ltr_srv_master.h"
#include "linuxtrack.h"
#include <time.h>
#include <sys/time.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <cal.h>
#include <ipc_utils.h>
#include <utils.h>
#include <axis.h>
#include <pref.h>

#include <map>
#include <string>

static std::multimap<std::string, int> slaves;
static semaphore_p pfSem = NULL;

static linuxtrack_full_pose_t current_pose;

static ltr_new_frame_callback_t new_frame_hook = NULL;
static ltr_status_update_callback_t status_update_hook = NULL;
static ltr_new_slave_callback_t new_slave_hook = NULL;

static bool save_prefs = true;
static bool no_slaves = false;
static pthread_mutex_t send_mx = PTHREAD_MUTEX_INITIALIZER;


bool ltr_int_gui_lock(bool do_lock)
{
  static const char *lockName = "ltr_server.lock";

  if((pfSem == NULL) ){
    //just check...
    switch(ltr_int_server_running_already(lockName, false, &pfSem, do_lock)){
      case 0:
        return true;
        break;
      case 1:
        ltr_int_log_message("Gui server running!");
        return false;
        break;
      default:
        ltr_int_log_message("Error locking gui server lock!");
        return false;
        break;
    }
  }else{
    if(do_lock){
      return ltr_int_tryLockSemaphore(pfSem);
    }else{
      return ltr_int_testLockSemaphore(pfSem);
    }
  }
}

void ltr_int_gui_lock_clean()
{
  if(pfSem != NULL){
    ltr_int_unlockSemaphore(pfSem);
    ltr_int_closeSemaphore(pfSem);
    pfSem= NULL;
  }
}

void ltr_int_change(const char *profile, int axis, int elem, float val)
{
  pthread_mutex_lock(&send_mx);
  std::pair<std::multimap<std::string, int>::iterator, std::multimap<std::string, int>::iterator> range;
  std::multimap<std::string, int>::iterator i;
  if(profile != NULL){
    //Finds all slaves belonging to the specific profile
    range = slaves.equal_range(profile);
    for(i = range.first; i != range.second; ++i){
      ltr_int_send_param_update(i->second, axis, elem, val);
    }
  }else{
    //Broadcast to all slaves
    for(i = slaves.begin(); i != slaves.end(); ++i){
      ltr_int_send_param_update(i->second, axis, elem, val);
    }
  }
  pthread_mutex_unlock(&send_mx);
}

//When slave is started, in GUI its axes might have changed (so better send through all )



void ltr_int_set_callback_hooks(ltr_new_frame_callback_t nfh, ltr_status_update_callback_t suh,
                                ltr_new_slave_callback_t nsh)
{
  new_frame_hook = nfh;
  status_update_hook = suh;
  new_slave_hook = nsh;
}

bool ltr_int_broadcast_pose(linuxtrack_full_pose_t &pose)
{
  pthread_mutex_lock(&send_mx);
  std::multimap<std::string, int>::iterator i;
  int res;
  bool checkSlaves = false;
  //Send updated pose to all clients
  for(i = slaves.begin(); i != slaves.end();){
    res = ltr_int_send_data(i->second, &pose);
    if(res == -EPIPE){
      ltr_int_log_message("Slave @fifo %d left!\n", i->second);
      close(i->second);
      i->second = -1;
      slaves.erase(i++);
      checkSlaves = true;
    }else{
      ++i;
    }
  }
  if(checkSlaves && (slaves.size() == 0)){
    no_slaves = true;
  }
  pthread_mutex_unlock(&send_mx);
  return true;
}

static void ltr_int_new_frame(struct frame_type *frame, void *param)
{
  (void)frame;
  (void)param;
  
  ltr_int_get_camera_update(&current_pose);
  //printf("CurrentPose=> p:%g y:%g r:%g\n", current_pose.pitch, current_pose.yaw, current_pose.roll);
  //printf("Master status: %d x %d\n", ltr_int_get_tracking_state(), current_pose.status);
  if(new_frame_hook != NULL){
    new_frame_hook(frame, (void*)&current_pose);
  }
  ltr_int_broadcast_pose(current_pose);
}

static void ltr_int_state_changed(void *param)
{
  (void)param;
  //ltr_int_log_message("State changed to %d\n", ltr_int_get_tracking_state());
  current_pose.pose.status = ltr_int_get_tracking_state();
  if(status_update_hook != NULL){
    status_update_hook(param);
  }
  ltr_int_broadcast_pose(current_pose);
}

bool ltr_int_register_slave(message_t &msg)
{
  printf("Trying to register slave!\n");
  char *tmp_fifo_name = NULL;
  if(asprintf(&tmp_fifo_name, ltr_int_slave_fifo_name(), msg.data) <= 0){
    return false;
  }
  std::string fifo_name(tmp_fifo_name);
  free(tmp_fifo_name);
  int fifo = ltr_int_open_fifo_for_writing(fifo_name.c_str(), true);
  if(fifo <= 0){
    return false;
  }
  pthread_mutex_lock(&send_mx);
  slaves.insert(std::pair<std::string, int>(msg.str, fifo));
  printf("Slave @fifo %d registered!\n", fifo);
  pthread_mutex_unlock(&send_mx);
  
  //Make sure the new section is created if needed...
  ltr_axes_t tmp_axes;
  tmp_axes = NULL;
  ltr_int_init_axes(&tmp_axes, msg.str);
  ltr_int_close_axes(&tmp_axes);
  
  if(save_prefs){
    ltr_int_log_message("Checking for changed prefs...\n");
    if(ltr_int_need_saving()){
      ltr_int_log_message("Master is about to save changed preferences.\n");
      ltr_int_save_prefs(NULL);
    }
  }

  if(new_slave_hook != NULL){
    new_slave_hook(msg.str);
  }
  return true;
}

void ltr_int_suspend_cmd()
{
  ltr_int_suspend();
}

void ltr_int_wakeup_cmd()
{
  ltr_int_wakeup();
}

void ltr_int_recenter_cmd()
{ 
  ltr_int_recenter();
}

static bool gui_shutdown_request = false;


size_t ltr_int_request_shutdown()
{
  //size_t res = slaves.size();
  //if(res == 0){
    gui_shutdown_request = true;
  //}
  return 0;
}

//Try making sure, that gui will be the only master
//  - opening and locking fifo in the gui constructor?
//  - when master running already, make it close to let us jump to its place???


bool ltr_int_master(bool standalone)
{
  current_pose.pose.pitch = 0.0;
  current_pose.pose.yaw = 0.0;
  current_pose.pose.roll = 0.0;
  current_pose.pose.tx = 0.0;
  current_pose.pose.ty = 0.0;
  current_pose.pose.tz = 0.0;
  current_pose.pose.counter = 0;
  current_pose.pose.status = STOPPED;
  current_pose.blobs = 0;
  gui_shutdown_request = false;
  int fifo;
  
  save_prefs = standalone;
  semaphore_p master_lock = NULL;
  if(standalone){
    //Detach from the caller, retaining stdin/out/err
    // Does weird things to gui ;)
    if(daemon(0, 1) != 0){
      return false;
    }
    if(!ltr_int_gui_lock(false)){
      printf("Gui is active, quitting!\n");
      return true;
    }
    fifo = ltr_int_open_fifo_exclusive(ltr_int_master_fifo_name(), &master_lock);
  }else{
    if(!ltr_int_gui_lock(true)){
      ltr_int_log_message("Couldn't lock gui lockfile!\n");
      return false;
    }
    int counter = 10;
    while((fifo = ltr_int_open_fifo_exclusive(ltr_int_master_fifo_name(), &master_lock)) <= 0){
      if((counter--) <= 0){
        ltr_int_log_message("The other master doesn't give up!\n");
        return false;
      }
      sleep(1);
    }
    ltr_int_log_message("Other master gave up, gui master taking over!\n");
  }
  
  if(fifo <= 0){
    printf("Master already running, quitting!\n");
    return true;
  }
  printf("Starting as master!\n");
  if(ltr_int_init() != 0){
    printf("Could not initialize tracking!\n");
    printf("Closing fifo %d\n", fifo);
    close(fifo);
    return false;
  }

  ltr_int_register_cbk(ltr_int_new_frame, NULL, ltr_int_state_changed, NULL);
  
  struct pollfd fifo_poll;
  int heartbeat = 0;
  fifo_poll.fd = fifo;
  fifo_poll.events = POLLIN;
  fifo_poll.revents = 0;
  no_slaves = false;
  while(1){
    fifo_poll.events = POLLIN;
    int fds = poll(&fifo_poll, 1, 1000);
    //printf("Master: poll returned %d @ %d!\n", fifo_poll.revents, fds);
    if(fds > 0){
      //ltr_int_log_message("poll: %d\n", fifo_poll.revents);
      if(fifo_poll.revents & POLLHUP){
        if(standalone){
          printf("We have HUP in Master!\n");
          linuxtrack_full_pose_t dummy;
          dummy.pose.pitch = 0.0; dummy.pose.yaw = 0.0; dummy.pose.roll = 0.0; 
          dummy.pose.tx = 0.0; dummy.pose.ty = 0.0; dummy.pose.tz = 0.0;
          dummy.pose.counter = 0; dummy.pose.status = PAUSED; dummy.blobs = 0;
          ltr_int_broadcast_pose(dummy);
        }else{
          //In gui when HUP comes, it goes forever...
          //!!! remove all clients!!!
          sleep(1);
        }
      }
      message_t msg;
      msg.cmd = CMD_NOP;
      ssize_t res = ltr_int_fifo_receive(fifo, &msg, sizeof(message_t));
      if(res < 0){
        ltr_int_my_perror("read");
      }
      if(res > 0){
        //printf("Received a message from slave (%d)!!!\n", msg.cmd);
        switch(msg.cmd){
          case CMD_PAUSE:
            ltr_int_suspend_cmd();
            break;
          case CMD_WAKEUP:
            ltr_int_wakeup_cmd();
            break;
          case CMD_RECENTER:
            ltr_int_recenter_cmd();
            break;
          case CMD_NEW_FIFO:
            //printf("Cmd to register new slave...\n");
            ltr_int_register_slave(msg);
            break;
        }
      }
    }else if(fds == 0){
      if(ltr_int_get_tracking_state() == PAUSED){
        ++heartbeat;
        if(heartbeat > 5){
          linuxtrack_full_pose_t dummy;
          dummy.pose.pitch = 0.0; dummy.pose.yaw = 0.0; dummy.pose.roll = 0.0; 
          dummy.pose.tx = 0.0; dummy.pose.ty = 0.0; dummy.pose.tz = 0.0;
          dummy.pose.counter = 0; dummy.pose.status = PAUSED; dummy.blobs = 0;
          ltr_int_broadcast_pose(dummy);
          heartbeat = 0;
        }
      }else{
        heartbeat = 0;
      }
    }else if(fds < 0){
      ltr_int_my_perror("poll");
    }
    
    if(gui_shutdown_request || (!ltr_int_gui_lock(false)) || 
       no_slaves || (ltr_int_get_tracking_state() < LINUXTRACK_OK)){
      break;
    }
  }
  printf("Shutting down tracking!\n");
  ltr_int_shutdown();
  printf("Master closing fifo %d\n", fifo);
  close(fifo);
  ltr_int_unlockSemaphore(master_lock);
  ltr_int_closeSemaphore(master_lock);
  ltr_int_gui_lock_clean();
  int cntr = 10;
  while((ltr_int_get_tracking_state() != STOPPED) && (cntr > 0)){
    --cntr;
    ltr_int_log_message("Tracker not stopped yet, waiting for the stop...\n");
    sleep(1);
  }
  ltr_int_gui_lock_clean();
  if(standalone){
    ltr_int_free_prefs();
  }
  return true;
}


