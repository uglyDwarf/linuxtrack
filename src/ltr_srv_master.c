#define _GNU_SOURCE
#include <stdio.h>
#include <malloc.h>
#include "ltr_srv_comm.h"

#include <pthread.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <cal.h>

typedef struct{
  char *profile_name;
  int fifo_number;
} slave_data_t; 

static bool stop_master_threads = false;
static pthread_cond_t new_data_cv = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t new_data_mx = PTHREAD_MUTEX_INITIALIZER;

static slave_data_t **sd_array = NULL;

bool sd_array_create()
{
  sd_array = (slave_data_t **)malloc(sizeof(slave_data_t*) * max_slave_fifos());
  if(sd_array == NULL){
    return false;
  }
  int i;
  for(i = 0; i < max_slave_fifos(); ++i){
    sd_array[i] = NULL;
  }
  return true;
}

bool sd_array_add_elem(slave_data_t *elem)
{
  int i;
  for(i = 0; i < max_slave_fifos(); ++i){
    if(sd_array[i] == NULL){
      sd_array[i] = elem;
      return true;
    }
  }
  return false;
}


//One of the most inefficient methods, but it should pose
//  no big problem, since it is being used only when GUI
//  is running and can be rewritten easily (c++ map,...)
int sd_array_find_elem(char *profile)
{
  int i;
  for(i = 0; i < max_slave_fifos(); ++i){
    if((sd_array[i] != NULL) && (strcmp(profile, (sd_array[i])->profile_name) == 0)){
      return (sd_array[i])->fifo_number;
    }
  }
  return false;
}

bool sd_array_free_elem(slave_data_t *elem)
{
  int i;
  for(i = 0; i < max_slave_fifos(); ++i){
    if(sd_array[i] == elem){
      free(elem->profile_name);
      free(elem);
      sd_array[i] = NULL;
      return true;
    }
  }
  return false;
}

bool sd_array_empty()
{
  int i;
  bool res = true;
  for(i = 0; i < max_slave_fifos(); ++i){
    res &= (sd_array[i] == NULL);
  }
  return res;
}

static pose_t current_pose;

ltr_new_frame_callback_t new_frame_hook = NULL;
ltr_status_update_callback_t status_update_hook = NULL;

void ltr_int_set_callback_hooks(ltr_new_frame_callback_t nfh, ltr_status_update_callback_t suh)
{
  new_frame_hook = nfh;
  status_update_hook = suh;
}

static void new_frame(struct frame_type *frame, void *param)
{
  (void)frame;
  (void)param;
  pthread_mutex_lock(&new_data_mx);
  ltr_int_get_camera_update(&(current_pose.yaw), &(current_pose.pitch), &(current_pose.roll), 
                            &(current_pose.tx), &(current_pose.ty), &(current_pose.tz), 
                            &(current_pose.counter));
  pthread_cond_broadcast(&new_data_cv);
  pthread_mutex_unlock(&new_data_mx);
  if(new_frame_hook != NULL){
    new_frame_hook(frame, (void*)&current_pose);
  }
}

static void state_changed(void *param)
{
  (void)param;
  pthread_mutex_lock(&new_data_mx);
  current_pose.status = ltr_int_get_tracking_state();
  pthread_cond_broadcast(&new_data_cv);
  pthread_mutex_unlock(&new_data_mx);
  if(status_update_hook != NULL){
    status_update_hook(param);
  }
}


void *master_worker_thread(void *param)
{
  (void) param;
  pthread_detach(pthread_self());
  ltr_int_register_cbk(new_frame, NULL, state_changed, NULL);
  while(!stop_master_threads){
    usleep(200000);
  }
  return 0;
}




void *master_writer_thread(void *param)
{
  pthread_detach(pthread_self());
  signal(SIGPIPE, SIG_IGN);
  slave_data_t *slave_data = (slave_data_t*)param;
  printf("Starting writer on slave fifo %02d for %s (%p)!\n", slave_data->fifo_number, 
                                                slave_data->profile_name, slave_data->profile_name);
  char *fifo_name = NULL;
  asprintf(&fifo_name, slave_fifo_name(), slave_data->fifo_number);
  int fifo = open_fifo_for_writing(fifo_name);
  if(fifo <= 0){
    return 0;
  }
  slave_data->fifo_number = fifo;
  if(!sd_array_add_elem(slave_data)){
    printf("Couldn't register! There will not be possibility to change prefs for %s (%p)!\n",
      slave_data->profile_name, slave_data->profile_name);
  }
  pose_t local_pose;
  while(!stop_master_threads){
    pthread_mutex_lock(&new_data_mx);
    while(local_pose.counter == current_pose.counter){
      pthread_cond_wait(&new_data_cv, &new_data_mx);
    }
    local_pose = current_pose;
    pthread_mutex_unlock(&new_data_mx);
    int res = send_data(fifo, &local_pose);
    if(res == EPIPE){
      break;
    }
  }
  if(!sd_array_free_elem(slave_data)){
    printf("Couldn't remove slave_data for %s!\n", slave_data->profile_name);
  }
  close(fifo);
  return 0;
}

bool master(bool daemonize)
{
  if(!sd_array_create()){
    return false;
  }
  //Open and lock the main communication fifo
  //  to make sure that only one master runs at a time. 
  int fifo = open_fifo_exclusive(master_fifo_name());
  if(fifo <= 0){
    printf("Master already running!\n");
    return true;
  }
  printf("Starting master!\n");
  if(daemonize){
    //Detach from the caller, retaining stdin/out/err
    daemon(0, 1);
  }
  if(ltr_int_init("Default") != 0){
    printf("Could not initialize tracking!\n");
    printf("Closing fifo %d\n", fifo);
    close(fifo);
    return false;
  }

  pthread_t worker_thread;
  pthread_create(&worker_thread, NULL, master_worker_thread, NULL);
  
  struct pollfd fifo_poll= {
    .fd = fifo,
    .events = POLLIN,
    .revents = 0
  };
  while(1){
    fifo_poll.events = POLLIN;
    int fds = poll(&fifo_poll, 1, 1000);
    if(fds > 0){
      if(fifo_poll.revents & POLLHUP){
        //printf("We have HUP in Master!\n");
        break;
      }
      message_t msg;
      pthread_t master_thread;
      if(fifo_receive(fifo, &msg, sizeof(message_t)) == 0){
        switch(msg.cmd){
          case CMD_PAUSE:
            ltr_int_suspend();
            break;
          case CMD_WAKEUP:
            ltr_int_wakeup();
            break;
          case CMD_RECENTER:
            ltr_int_recenter();
            break;
          case CMD_NEW_FIFO:
            stop_master_threads = false;
            slave_data_t *sd = malloc(sizeof(slave_data_t));
            sd->fifo_number = msg.data;
            asprintf(&(sd->profile_name), "%s", msg.str);
            printf("Profile %s (%p)\n", sd->profile_name, sd->profile_name);
            pthread_create(&master_thread, NULL, master_writer_thread, (void*)sd);
            break;
        }
      }
    }else if(fds < 0){
      perror("poll");
    }
  }
  printf("Shutting down tracking!\n");
  ltr_int_shutdown();
  printf("Closing fifo %d\n", fifo);
  close(fifo);
  int i = 10;
  while(!sd_array_empty()){
    usleep(100000);
    --i;
    if(i < 0){
      break;
    }
  }
  if(sd_array != NULL){
    free(sd_array);
    sd_array = NULL;
  }
  return true;
}


