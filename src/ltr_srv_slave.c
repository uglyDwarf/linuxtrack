
#include "ltr_srv_comm.h"
#include <stdio.h>
#include <wait.h>
#include <poll.h>
#include <pthread.h>
#include <unistd.h>
#include <malloc.h>
#include <ipc_utils.h>
#include <ltlib.h>
#include <utils.h>
#include <tracking.h>
#include <pref_int.h>

static char *profile_name = NULL;
static struct mmap_s mmm;
static bool try_restarting_master = true;
static bool slave_reader_stopped = true;

int open_slave_fifo(int master_fifo, const char *name_template, int max_fifos)
{
  char *data_fifo_name = NULL;
  //Open the data passing fifo and pass it to the master...
  int fifo_number = -1;
  int data_fifo = open_unique_fifo(&data_fifo_name, &fifo_number, name_template, max_fifos);
  if(data_fifo <= 0){
    return -1;
  }
  if(send_message_w_str(master_fifo, CMD_NEW_FIFO, fifo_number, profile_name) == 0){
    return data_fifo;
  }else{
    close(data_fifo);
    return -1;
  }
}

void try_start_master(const char *main_fifo)
{
  if(!try_restarting_master){
    return;
  }
  int fifo = open_fifo_exclusive(main_fifo);
  if(fifo > 0){
    close(fifo);
    printf("Master is not running, start it\n"); 
    char *args[] = {"srv", NULL};
    args[0] = ltr_int_get_app_path("/ltr_server1");
    ltr_int_fork_child(args);
    int status;
    wait(&status);
  }
}

bool try_restart_master(struct pollfd *fifo_poll, int *fifo, int *master_fifo)
{
  static int restart_counter = 10;
  --restart_counter;
  if(restart_counter < 0){
    printf("Master restarted too many times, giving up!");
    return false;
  }
  printf("Trying to restart master!\n");
  //Close the data fifo before starting new master, otherwise master will retain it!
  close(*fifo);
  close(*master_fifo);
  *master_fifo = -1;
  try_start_master(master_fifo_name());
  *master_fifo = open_fifo_for_writing(master_fifo_name());
  if(*master_fifo <= 0){
    printf("Couldn't open fifo to master!\n");
    return false;
  }
  if((*fifo = open_slave_fifo(*master_fifo, slave_fifo_name(), max_slave_fifos())) <= 0){
    printf("Couldn't pass master our fifo!\n");
    return false;
  }
  fifo_poll->fd = *fifo;
  return true;
}

static int master_fifo = -1;
static bool stop_slave_reader_thread = false;
static ltr_axes_t axes;

void *slave_reader_thread(void *param)
{
  (void) param;
  slave_reader_stopped = false;
  pthread_detach(pthread_self());
  int fifo;
  if((fifo = open_slave_fifo(master_fifo, slave_fifo_name(), max_slave_fifos())) <= 0){
    printf("Couldn't pass master our fifo!\n");
    return 0;
  }
  struct pollfd fifo_poll= {
    .fd = fifo,
    .events = POLLIN,
    .revents = 0
  };
  printf("Fifo %d\n", fifo);
  
  while(!stop_slave_reader_thread){
    fifo_poll.events = POLLIN;
    int fds = poll(&fifo_poll, 1, 1000);
    if(fds > 0){
      if(fifo_poll.revents & POLLHUP){
        if(!try_restart_master(&fifo_poll, &fifo, &master_fifo)){
          return false;
        }
      }
      message_t msg;
      struct ltr_comm *com;
      if(fifo_receive(fifo, &msg, sizeof(message_t)) == 0){
        switch(msg.cmd){
          case CMD_POSE:
            //printf("Have new pose!\n");
            ltr_int_postprocess_axes(axes, &(msg.pose));
            
            com = mmm.data;
            ltr_int_lockSemaphore(mmm.sem);
            com->heading = msg.pose.yaw;
            com->pitch = msg.pose.pitch;
            com->roll = msg.pose.roll;
            com->tx = msg.pose.tx;
            com->ty = msg.pose.ty;
            com->tz = msg.pose.tz;
            com->counter = msg.pose.counter;
            com->state = msg.pose.status;
            ltr_int_unlockSemaphore(mmm.sem);
            break;
          case CMD_PARAM:
            printf("Updating %d@%d to %f\n", msg.param.axis_id, msg.param.param_id, msg.param.flt_val);
            break;
          default:
            printf("Slave received unexpected message!\n");
            break;
        }
        //printf("Received: '%s'\n", msg.str);
      }else{
        printf("slave reader problem!\n");
      }
    }else if(fds < 0){
      perror("poll");
    }
    
  }
  close(fifo);
  ltr_int_unmap_file(&mmm);
  slave_reader_stopped = true;
  return 0;
}


bool slave(const char *c_profile, bool restart_master)
{
  try_restarting_master = restart_master;
  char *profile = ltr_int_my_strdup(c_profile);
  char *com_file = ltr_int_get_com_file_name();
  if(!ltr_int_mmap_file(com_file, sizeof(struct ltr_comm), &mmm)){
    printf("Couldn't mmap file!!!\n");
    return false;
  }
  free(com_file);
  struct ltr_comm *com = mmm.data;
  if(!ltr_int_read_prefs(NULL, false)){
    printf("Couldn't load preferences!\n");
    return false;
  }
  ltr_int_set_custom_section(profile);
  ltr_int_init_axes(&axes);
  if(master_fifo != -1){
    close(master_fifo);
  }
  try_start_master(master_fifo_name());
  
  profile_name = profile;
  printf("Starting slave (profile: %s)!\n", profile_name);
  
  master_fifo = open_fifo_for_writing(master_fifo_name());
  if(master_fifo <= 0){
    return false;
  }

  //create reader thread (gets data from master andprocesses them).
  pthread_t reader_tid;
  stop_slave_reader_thread = false;
  pthread_create(&reader_tid, NULL, slave_reader_thread, NULL);
  ltr_cmd cmd = NOP_CMD;
  bool recenter;
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
        send_message(master_fifo, CMD_PAUSE, 0);
        break;
      case RUN_CMD:
        send_message(master_fifo, CMD_WAKEUP, 0);
        break;
      case STOP_CMD:
        quit_flag = true;
        break;
      default:
        break;
    }
    cmd = NOP_CMD;
    if(recenter){
      printf("Slave sending master recenter request!\n");
      send_message(master_fifo, CMD_RECENTER, 0);
    }
    recenter = false;
    if(getppid() == 1){
      printf("Parent died!\n");
      break;
    }
    usleep(100000);
  }
  slave_reader_stopped = false;
  stop_slave_reader_thread = true;
  int to = 20;
  while((!slave_reader_stopped)&&(to > 0)){
    usleep(100000);
    --to;
  }
  printf("Closing fifo %d\n", master_fifo);
  close(master_fifo);
  master_fifo = -1;
  return true;
}


