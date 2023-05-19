#include <pthread.h>
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
#include <sys/socket.h>
#include <sys/un.h>

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

  if(pfSem == NULL){
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
  //printf("Master: %g  %g  %g\n", pose.pose.raw_pitch, pose.pose.raw_yaw, pose.pose.raw_roll);
  for(i = slaves.begin(); i != slaves.end();){
    res = ltr_int_send_data(i->second, &pose);
    if(res == -EPIPE){
      ltr_int_log_message("Slave @socket %d left!\n", i->second);
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
  //printf("CurrentPose=> p:%g y:%g r:%g\n", current_pose.pose.pitch, current_pose.pose.yaw,
  //       current_pose.pose.roll);
  //printf("Master status: %d x %d\n", ltr_int_get_tracking_state(), current_pose.pose.status);
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

bool ltr_int_register_slave(int socket, message_t &msg)
{
  ltr_int_log_message("Trying to register slave!\n");
  pthread_mutex_lock(&send_mx);
  slaves.insert(std::pair<std::string, int>(msg.str, socket));
  ltr_int_log_message("Slave with profile '%s' @socket %d registered!\n", msg.str, socket);
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



struct pollfd *descs = NULL;
size_t max_len = 0;
size_t current_len = 0;
nfds_t numfd = 0;

bool add_poll_desc(int fd){
  ltr_int_log_message("Adding fd %d\n", fd);
  if(current_len >= max_len){
    if(max_len > 0){
      max_len *= 2;
      descs = (struct pollfd*)realloc(descs, max_len * sizeof(struct pollfd));
      if(descs){
        memset(descs + current_len, 0,
               (max_len - current_len) * sizeof(struct pollfd));//???
      }
    }else{
      max_len = 1;
      descs = (struct pollfd*)malloc(max_len * sizeof(struct pollfd));
      if(descs){
        memset(descs, 0, max_len * sizeof(struct pollfd));
      }
    }
    if(descs == NULL){
      ltr_int_my_perror("m/re-alloc");
      ltr_int_log_message("Can't alloc enough memory for the poll!\n");
      exit(1);
    }
  }
  descs[current_len].fd = fd;
  descs[current_len].events = POLLIN;
  descs[current_len].revents = 0;
  ++current_len;
  return true;
}

void print_descs(void)
{
  nfds_t i;
  for(i = 0; i < current_len; ++i){
    ltr_int_log_message("%ld: fd=%d, ev=%d, rev=%d\n", (long int)i, 
      descs[i].fd, descs[i].events, descs[i].revents);
  }
}

bool remove_poll_desc(){
  nfds_t i;
  //ltr_int_log_message("Watched descriptors before garbage collection.\n");
  //print_descs();
  for(i = 0; i < current_len; ++i){
    if(descs[i].revents & POLLHUP){
      ltr_int_log_message("Removing hanged fd %d\n", descs[i].fd);
      if(i < (current_len - 1)){
        descs[i] = descs[current_len - 1];
      }else{
        //last fd, just decrement current_len
        // Intentionaly empty
      }
      --current_len;
    }
  }
  //ltr_int_log_message("Watched descriptors after garbage collection.\n");
  //print_descs();
  return true;
}


int ltr_int_master_main_loop(int socket)
{
  int res;
  int new_fd;
  bool close_conn;
  int heartbeat = 0;
  nfds_t i;
  no_slaves = false;

  struct sockaddr_un address;
  socklen_t address_len = sizeof(address);
  add_poll_desc(socket);
  while(1){
    if(gui_shutdown_request || (!ltr_int_gui_lock(false)) ||
       no_slaves || (ltr_int_get_tracking_state() < LINUXTRACK_OK)){
      break;
    }

    close_conn = false;
    for(i = 0; i < current_len; ++i){
      descs[i].revents = 0;
    }
    res = poll(descs, current_len, 2000);
    if(res < 0){
      ltr_int_my_perror("poll");
      continue;
    }else if(res == 0){
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
      continue;
    }
    for(i = 0; i < current_len; ++i){
      if(descs[i].revents == 0){
        continue;
      }
      if(descs[i].revents){
        if(descs[i].revents & POLLIN){
          if(descs[i].fd == socket){
            do{
              new_fd = accept(socket, (struct sockaddr*)&address, &address_len);
              if(new_fd < 0){
                if(errno != EWOULDBLOCK){
                  ltr_int_my_perror("accept");
                }
                //No more connection requests
                break;
              }else{
                add_poll_desc(new_fd);
              }
            }while(new_fd >= 0);
          }else{
            message_t msg;
            msg.cmd = CMD_NOP;
            ssize_t x = ltr_int_socket_receive(descs[i].fd, &msg, sizeof(message_t));
            //ltr_int_log_message("Read %d bytes from fd %d.\n", (int)x, descs[i].fd);
            if(x < 0){
              if(x != -EWOULDBLOCK){
                ltr_int_log_message("Unexpected error %d reading from fd %d.\n", x, descs[i].fd);
                close_conn = true;
                descs[i].fd = -1;
              }
            }else if(x == 0){
              //close_conn = true;
              //descs[i].fd = -1;
            }else{
              //ltr_int_log_message("Received a message from slave (%d)!!!\n", msg.cmd);
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
                case CMD_NEW_SOCKET:
                  //ltr_int_log_message("Cmd to register new slave...\n");
                  ltr_int_register_slave(descs[i].fd, msg);
                  break;
                case CMD_FRAMES:
                  ltr_int_publish_frames_cmd();
                  break;
              }
            }
          }
        }
        if(descs[i].revents & POLLHUP){
          ltr_int_log_message("Hangup at fd %d\n", descs[i].fd);
          close_conn = true;
        }
      }
    }
    if(close_conn){
      remove_poll_desc();
    }
  }
  return 0;
}



//Try making sure, that gui will be the only master
//  - opening and locking socket in the gui constructor?
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
  int socket;

  save_prefs = standalone;
  if(standalone){
    //Detach from the caller, retaining stdin/out/err
    // Does weird things to gui ;)
    if(daemon(0, 1) != 0){
      return false;
    }
    if(!ltr_int_gui_lock(false)){
      ltr_int_log_message("Gui is active, quitting!\n");
      return true;
    }
    socket = ltr_int_create_server_socket(ltr_int_master_socket_name());
  }else{
    if(!ltr_int_gui_lock(true)){
      ltr_int_log_message("Couldn't lock gui lockfile!\n");
      return false;
    }
    int counter = 10;
    while((socket = ltr_int_create_server_socket(ltr_int_master_socket_name())) <= 0){
      if((counter--) <= 0){
        ltr_int_log_message("The other master doesn't give up!\n");
        return false;
      }
      sleep(1);
    }
    ltr_int_log_message("Other master gave up, gui master taking over!\n");
  }

  if(socket < 0){
    ltr_int_log_message("Master already running, quitting!\n");
    return true;
  }
  ltr_int_log_message("Starting as master!\n");
  if(ltr_int_init() != 0){
    ltr_int_log_message("Could not initialize tracking!\n");
    ltr_int_log_message("Closing socket %d\n", socket);
    close(socket);
    unlink(ltr_int_master_socket_name());
    return false;
  }

  ltr_int_register_cbk(ltr_int_new_frame, NULL, ltr_int_state_changed, NULL);

  ltr_int_master_main_loop(socket);

  ltr_int_log_message("Shutting down tracking!\n");
  ltr_int_shutdown();
  ltr_int_log_message("Master closing socket %d\n", socket);
  close(socket);
  unlink(ltr_int_master_socket_name());
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


