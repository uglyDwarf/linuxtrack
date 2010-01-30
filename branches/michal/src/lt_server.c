/*
 * This is the server part of linuxtrack.
 * Its responsibility is to get frames from the device and send them
 * over to the client part.
 *
 * It has four states:
 *  1) IDLE - waiting for incomming connection
 *  2) SUSPENDED - device is initialized but stopped
 *  3) WORKING - capturing frames and sending them to the client
 *
 *
 *
 */

#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include "netcomm.h"
#include "cal.h"
#include "utils.h"
#include "pref_global.h"

pthread_t data_relay_thread;

pthread_cond_t state_cv;
pthread_mutex_t state_mx;

static struct camera_control_block ccb;

enum {RUN, PAUSE, STOP} thread_state = STOP;


void *data_relay(void *arg)
{
  char msg[1024];
  int sfd = *(int *)arg;
  int cntr = 0;
  int retval;
  struct frame_type frame;
  bool was_running;
  
  printf("Initializing data_relay thread!\n");
  
  if(get_device(&ccb) == false){
    log_message("Can't get device category!\n");
    return NULL;
  }
  ccb.mode = operational_3dot;
  ccb.diag = false;
  if(cal_init(&ccb)!= 0){
    return NULL;
  }
  frame.bloblist.blobs = my_malloc(sizeof(struct blob_type) * 3);
  frame.bloblist.num_blobs = 3;
  frame.bitmap = NULL;
  
  was_running = true;
  while(1){
    pthread_mutex_lock(&state_mx);
    switch(thread_state){
      case RUN:
        pthread_mutex_unlock(&state_mx);
	retval = cal_get_frame(&ccb, &frame);
	printf("Have frame!\n");
	snprintf(msg, sizeof(msg), "New val %d....\n", cntr++);
	write(sfd, &msg, sizeof(msg));
	was_running = true;
        break;
      case PAUSE:
        if(was_running == true){
	  cal_suspend(&ccb);
	  was_running =false;
	}
	while(thread_state == PAUSE){
	  pthread_cond_wait(&state_cv, &state_mx);
	}
	pthread_mutex_unlock(&state_mx);
	cal_wakeup(&ccb);
	break;
      default:
        break;
    }
    if(thread_state == STOP){
      break;
    }
  }
  printf("Thread stopping\n");
  if(ccb.state != active){
    cal_wakeup(&ccb);
  }
  cal_shutdown(&ccb);
  frame_free(&ccb, &frame);
  printf("Thread almost stopped\n");
  return NULL;
}


int the_server_thing(int sfd)
{
  int cfd;
  while(1){
    cfd = accept_connection(sfd);
    if(cfd == -1){
      perror("accept:");
      continue;
    }
    char msg = SHUTDOWN;
    ssize_t ret;
    do{
      ret = read(cfd, &msg, sizeof(msg));
      switch(msg){
        case INIT:
	  printf("INIT\n");
	  if(thread_state == STOP){
	    thread_state = RUN;
	    if(pthread_create(&data_relay_thread, NULL, data_relay, &cfd) != 0){
	      thread_state = STOP;
	    }
	  }
	  break;
        case SHUTDOWN:
	  printf("SHUTDOWN\n");
	  pthread_mutex_lock(&state_mx);
	  switch(thread_state){
	    case RUN:
	      thread_state = STOP;
	      break;
	    case PAUSE:
	      thread_state = STOP;
	      pthread_cond_signal(&state_cv);
	      break;
	    default:
	      break;
	  }
	  pthread_mutex_unlock(&state_mx);
	  break;
        case SUSPEND:
	  printf("SUSPEND\n");
	  if(thread_state == RUN){
	    pthread_mutex_lock(&state_mx);
	    thread_state = PAUSE;
	    pthread_mutex_unlock(&state_mx);
	  }
	  break;
        case WAKE:
	  printf("WAKEUP\n");
	  pthread_mutex_lock(&state_mx);
	  if(thread_state == PAUSE){
	    thread_state = RUN;
	    pthread_cond_signal(&state_cv);
	  }
	  pthread_mutex_unlock(&state_mx);
	  break;
	default:
	  printf("Unknown message!!!\n");
	  break;
      }
    }while(msg != SHUTDOWN);
    
    close(cfd);
  }
  return 0;
}


int main(int argc, char *argv[])
{
  if(argc != 2){
    fprintf(stderr, "Bad args...\n");
    return 1;
  }
  int sfd;
  
  if(pthread_mutex_init(&state_mx, NULL)){
    fprintf(stderr, "Can't init mutex!\n");
    return 1;
  }
  if(pthread_cond_init(&state_cv, NULL)){
    fprintf(stderr, "Can't init cond. var.!\n");
    return 1;
  }
  
  if((sfd = init_server(atoi(argv[1]))) < 0){
    fprintf(stderr, "Have problem....\n");
    return 1;
  }
  
  
  the_server_thing(sfd);
  pthread_cond_destroy(&state_cv);
  printf("Seems to work...\n");
  sleep(1);
  return 0;
}
