/*
 * This is the server part of linuxtrack.
 * Its responsibility is to get frames from the device and send them
 * over to the client part.
 */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include "netcomm.h"
#include "cal.h"
#include "utils.h"
#include "pref_global.h"

pthread_cond_t state_cv;
pthread_mutex_t state_mx;
enum {WAITING, WORKING, SHUTTING} state = WAITING;
pthread_t comm;

static struct camera_control_block ccb;
int sfd;
int cfd;

int frame_callback(struct camera_control_block *ccb, struct frame_type *frame)
{
  assert(cfd > 0);
  assert(frame->bloblist.num_blobs);
  unsigned char msg[2048];
  
  size_t size = encode_bloblist(&(frame->bloblist), msg);
  assert(cfd > 0);
  if(send(cfd, &msg, size, MSG_NOSIGNAL) < 0){
    perror("write:");
    return -1;
  }
  return 0;
}

void* the_server_thing(void *param)
{
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
      if(ret==-1){
        perror("read");
	continue;
      }
      switch(msg){
        case RUN:
          pthread_mutex_lock(&state_mx);
          state = WORKING;
          pthread_cond_broadcast(&state_cv);
          pthread_mutex_unlock(&state_mx);
         break;
        case SHUTDOWN:
          pthread_mutex_lock(&state_mx);
          state = SHUTTING;
          pthread_cond_broadcast(&state_cv);
          pthread_mutex_unlock(&state_mx);
          cal_shutdown();
	  break;
        case SUSPEND:
          cal_suspend();
	  break;
        case WAKE:
          cal_wakeup();
	  break;
	default:
          assert(0);
	  break;
      }
    }while(msg != SHUTDOWN);
    
    pthread_mutex_lock(&state_mx);
    while(state != WAITING){
      pthread_cond_wait(&state_cv, &state_mx);
    }
    pthread_mutex_unlock(&state_mx);
    close(cfd);
    cfd = -1;
  }
  return NULL;
}


int main(int argc, char *argv[])
{
  if(argc != 2){
    log_message("Bad args...\n");
    return 1;
  }
  
  if((sfd = init_server(atoi(argv[1]))) < 0){
    log_message("Have problem....\n");
    return 1;
  }
  
  if(pthread_mutex_init(&state_mx, NULL)){
    log_message("Can't init mutex!\n");
    return 1;
  }
  if(pthread_cond_init(&state_cv, NULL)){
    log_message("Can't init cond. var.!\n");
    return 1;
  }
  
  
  pthread_create(&comm, NULL, the_server_thing, NULL);
  
  while(1){
    pthread_mutex_lock(&state_mx);
    
    while(state == WAITING){
      pthread_cond_wait(&state_cv, &state_mx);
    }
    pthread_mutex_unlock(&state_mx);

    if(get_device(&ccb) == false){
      log_message("Can't get device category!\n");
      return 1;
    }
    ccb.mode = operational_3dot;
    ccb.diag = false;
    cal_run(&ccb, frame_callback);
    
    pthread_mutex_lock(&state_mx);
    state = WAITING;
    pthread_cond_broadcast(&state_cv);
    pthread_mutex_unlock(&state_mx);
  }
  pthread_cond_destroy(&state_cv);
  return 0;
}
