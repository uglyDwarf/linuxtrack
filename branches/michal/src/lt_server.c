/*
 * This is the server part of linuxtrack.
 * Its responsibility is to get frames from the device and send them
 * over to the client part.
 *
 *
 *
 *
 */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include "netcomm.h"
#include "cal.h"
#include "utils.h"
#include "pref_global.h"

bool waiting = true;
pthread_cond_t state_cv;
pthread_mutex_t state_mx;
pthread_t comm;

static struct camera_control_block ccb;
int sfd;
int cfd;

int frame_callback(struct camera_control_block *ccb, struct frame_type *frame)
{
  //BEWARE OF OVERRUN!
  unsigned char msg[2048];
  printf("Have frame!\n");
  printf("%d blobs!\n", frame->bloblist.num_blobs);
  printf("[%g, %g]\n", (frame->bloblist.blobs[0]).x, (frame->bloblist.blobs[0]).y);
  printf("[%g, %g]\n", (frame->bloblist.blobs[1]).x, (frame->bloblist.blobs[1]).y);
  printf("[%g, %g]\n\n", (frame->bloblist.blobs[2]).x, (frame->bloblist.blobs[2]).y);
  size_t size = encode_bloblist(&(frame->bloblist), msg);
  if(send(cfd, &msg, size, MSG_NOSIGNAL) < 0){
    perror("write:");
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
          waiting = false;
          pthread_cond_broadcast(&state_cv);
          pthread_mutex_unlock(&state_mx);
         break;
        case SHUTDOWN:
          cal_shutdown();
	  break;
        case SUSPEND:
          cal_suspend();
	  break;
        case WAKE:
          cal_wakeup();
	  break;
	default:
	  log_message("Unknown message!!!\n");
	  break;
      }
    }while(msg != SHUTDOWN);
    
    close(cfd);
  }
  return NULL;
}


int main(int argc, char *argv[])
{
  if(argc != 2){
    fprintf(stderr, "Bad args...\n");
    return 1;
  }
  
  if((sfd = init_server(atoi(argv[1]))) < 0){
    fprintf(stderr, "Have problem....\n");
    return 1;
  }
  
  if(pthread_mutex_init(&state_mx, NULL)){
    fprintf(stderr, "Can't init mutex!\n");
    return 1;
  }
  if(pthread_cond_init(&state_cv, NULL)){
    fprintf(stderr, "Can't init cond. var.!\n");
    return 1;
  }
  
  
  pthread_create(&comm, NULL, the_server_thing, NULL);
  
  while(1){
    pthread_mutex_lock(&state_mx);
    
    while(waiting == true){
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
    waiting = true;
  }
  pthread_cond_destroy(&state_cv);
  return 0;
}
