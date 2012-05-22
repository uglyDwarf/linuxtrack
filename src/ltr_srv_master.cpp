#include <stdio.h>
#include <malloc.h>
#include "ltr_srv_comm.h"
#include "ltr_srv_master.h"
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

#include <map>
#include <string>

std::multimap<std::string, int> slaves;

static pose_t current_pose;

ltr_new_frame_callback_t new_frame_hook = NULL;
ltr_status_update_callback_t status_update_hook = NULL;

static const char *lockName = "ltr_server.lock";
static semaphore_p pfSem = NULL;

bool ltr_int_gui_lock()
{
  switch(ltr_int_server_running_already(lockName, &pfSem, true)){
    case 0:
      return true;
      break;
    case 1:
      ltr_int_log_message("Gui server runs already!");
      return false;
      break;
    default:
      ltr_int_log_message("Error locking gui server lock!");
      return false;
      break;
  }
}

bool ltr_int_gui_lock_active()
{
  if(pfSem == NULL){
    switch(ltr_int_server_running_already(lockName, &pfSem, false)){
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
    return ltr_int_testLockSemaphore(pfSem);
  }
}

void change(const char *profile, int axis, int elem, float val)
{
  std::pair<std::multimap<std::string, int>::iterator, std::multimap<std::string, int>::iterator> range;
  std::multimap<std::string, int>::iterator i;
  range = slaves.equal_range(profile);
  for(i = range.first; i != range.second; ++i){
    send_param_update(i->second, axis, elem, val);
  }
}

//When slave is started, in GUI its axes might have changed



void ltr_int_set_callback_hooks(ltr_new_frame_callback_t nfh, ltr_status_update_callback_t suh)
{
  new_frame_hook = nfh;
  status_update_hook = suh;
}

bool broadcast_pose(pose_t &pose)
{
  bool prune = false;
  std::multimap<std::string, int>::iterator i;
  int res;
  //Send updated pose to all clients
  for(i = slaves.begin(); i != slaves.end(); ++i){
    res = send_data(i->second, &pose);
    if(res == EPIPE){
      printf("Slave @fifo %d left!\n", i->second);
      close(i->second);
      i->second = -1;
      prune = true;
    }
  }
  //When some of clients exit, clean up
  if(prune){
    for(i = slaves.begin(); i != slaves.end(); ++i){
      if(i->second == -1){
        slaves.erase(i);
      }
    }
  }
  return true;
}

static void new_frame(struct frame_type *frame, void *param)
{
  (void)frame;
  (void)param;
  //TODO send data to all slaves!
  //printf("Have new pose - master!\n");
  ltr_int_get_camera_update(&(current_pose.yaw), &(current_pose.pitch), &(current_pose.roll), 
                            &(current_pose.tx), &(current_pose.ty), &(current_pose.tz), 
                            &(current_pose.counter));
  if(new_frame_hook != NULL){
    new_frame_hook(frame, (void*)&current_pose);
  }
  broadcast_pose(current_pose);
}

static void state_changed(void *param)
{
  (void)param;
  current_pose.status = ltr_int_get_tracking_state();
  if(status_update_hook != NULL){
    status_update_hook(param);
  }
  broadcast_pose(current_pose);
}

bool register_slave(message_t &msg)
{
  printf("Trying to register slave!\n");
  char *tmp_fifo_name = NULL;
  if(asprintf(&tmp_fifo_name, slave_fifo_name(), msg.data) <= 0){
    return false;
  }
  std::string fifo_name(tmp_fifo_name);
  free(tmp_fifo_name);
  int fifo = open_fifo_for_writing(fifo_name.c_str());
  if(fifo <= 0){
    return false;
  }
  printf("Slave @fifo %d registered!\n", fifo);
  slaves.insert(std::pair<std::string, int>(msg.str, fifo));
  return true;
}

void suspend_cmd()
{
  ltr_int_suspend();
}

void wakeup_cmd()
{
  ltr_int_wakeup();
}

void recenter_cmd()
{ 
  ltr_int_recenter();
}

bool gui_shutdown_request = false;

size_t request_shutdown()
{
  size_t res = slaves.size();
  if(res == 0){
    gui_shutdown_request = true;
  }
  return res;
}

//Try making sure, that gui will be the only master
//  - opening and locking fifo in the gui constructor?
//  - when master running already, make it close to let us jump to its place???


bool master(bool standalone)
{
  gui_shutdown_request = false;
  int fifo;
  
  if(standalone){
    if(!ltr_int_gui_lock_active()){
      printf("Gui is active, quitting!\n");
      return true;
    }
    fifo = open_fifo_exclusive(master_fifo_name());
  }else{
    if(!ltr_int_gui_lock()){
      ltr_int_log_message("Couldn't lock gui lock!");
      return false;
    }
    int counter = 10;
    while((fifo = open_fifo_exclusive(master_fifo_name())) <= 0){
      if((counter--) <= 0){
        ltr_int_log_message("The other master doesn't give up!");
        break;
      }
      sleep(1);
    }
    ltr_int_log_message("Other master gave up, gui master taking over!");
  }
  
  //Open and lock the main communication fifo
  //  to make sure that only one master runs at a time. 
  if(fifo <= 0){
    printf("Master already running, quitting!\n");
    return true;
  }
  printf("Starting as master!\n");
  if(standalone){
    //Detach from the caller, retaining stdin/out/err
    if(daemon(0, 1) != 0){
      return false;
    }
  }
  if(ltr_int_init(NULL) != 0){
    printf("Could not initialize tracking!\n");
    printf("Closing fifo %d\n", fifo);
    close(fifo);
    return false;
  }

  ltr_int_register_cbk(new_frame, NULL, state_changed, NULL);
  
  struct pollfd fifo_poll;
  fifo_poll.fd = fifo;
  fifo_poll.events = POLLIN;
  fifo_poll.revents = 0;
  
  while(1){
    fifo_poll.events = POLLIN;
    int fds = poll(&fifo_poll, 1, 1000);
    if(fds > 0){
      if(fifo_poll.revents & POLLHUP){
        //printf("We have HUP in Master!\n");
        break;
      }
      message_t msg;
      if(fifo_receive(fifo, &msg, sizeof(message_t)) == 0){
        switch(msg.cmd){
          case CMD_PAUSE:
            if(standalone){
              suspend_cmd();
            }
            break;
          case CMD_WAKEUP:
            if(standalone){
              wakeup_cmd();
            }
            break;
          case CMD_RECENTER:
            if(standalone){
              recenter_cmd();
            }
            break;
          case CMD_NEW_FIFO:
            register_slave(msg);
            break;
        }
      }
    }else if(fds < 0){
      perror("poll");
    }
    
    if(gui_shutdown_request || (!ltr_int_gui_lock_active())){
      break;
    }
    
  }
  printf("Shutting down tracking!\n");
  ltr_int_shutdown();
  printf("Master closing fifo %d\n", fifo);
  close(fifo);
  return true;
}


