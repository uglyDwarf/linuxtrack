#include <pthread.h>
#include "netcomm.h"
#include "utils.h"
#include "tracking.h"
#include <stdio.h> //TODO
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>

pthread_t data_receiver_thread;
int cfd;
bool close_receiver;




void *lt_data_receiver(void *arg)
{
  struct frame_type frame;
  struct blob_type nbt[3];
  frame.bloblist.blobs = nbt;
  frame.bitmap = NULL;
  cfd = *(int *)arg;
  printf("Data receiver starting!\n");
  ssize_t ret;
  unsigned char msg[1024];
  fd_set rd;
  struct timeval tv;
  while((!close_receiver) && (cfd > 0)){
  FD_ZERO(&rd);
  FD_SET(cfd, &rd);
    tv.tv_sec = 0,
    tv.tv_usec = 200000;
    int r = select(cfd + 1, &rd, NULL, NULL, &tv);
    if((r == -1) && (errno == EINTR)){
        continue;
    }
    if(r < 0){
        perror("select()");
        exit(EXIT_FAILURE);
    }
    if (FD_ISSET(cfd, &rd)) {
      ret = read(cfd, &msg, sizeof(msg));
      if(ret > 0){
        decode_bloblist(&(frame.bloblist), msg);
        update_pose(&frame);
        log_message("%d blobs!\n", frame.bloblist.num_blobs); //TODO
        log_message("[%g, %g]\n", (frame.bloblist.blobs[0]).x, (frame.bloblist.blobs[0]).y);
        log_message("[%g, %g]\n", (frame.bloblist.blobs[1]).x, (frame.bloblist.blobs[1]).y);
        log_message("[%g, %g]\n\n", (frame.bloblist.blobs[2]).x, (frame.bloblist.blobs[2]).y);
      }
    }
    
  }
  close(cfd);
  cfd = -1;
  return NULL;
}

int lt_client_init()
{
  int sfd;
  message_t msg;
  if((sfd = init_client("localhost", 3004)) < 0){ //TODO
    log_message("Can't initialize client!\n");
    return 1;
  }
  close_receiver = false;
  pthread_create(&data_receiver_thread, NULL, lt_data_receiver, &sfd);
  msg = RUN;
  if(write(cfd, &msg, sizeof(msg)) != sizeof(msg)){
    return -1;
  }
  log_message("Initializing...\n");
  return 0;
}

int lt_client_suspend()
{
  message_t msg;
  msg = SUSPEND;
  if(write(cfd, &msg, sizeof(msg)) != sizeof(msg)){
    return -1;
  }
  log_message("Suspending...\n");
  return 0;
}

int lt_client_wakeup()
{
  message_t msg;
  msg = WAKE;
  if(write(cfd, &msg, sizeof(msg)) != sizeof(msg)){
    return -1;
  }
  log_message("Waking...\n");
  return 0;
}

int lt_client_close()
{
  message_t msg;
  close_receiver = true;
  msg = SHUTDOWN;
  if(write(cfd, &msg, sizeof(msg)) == sizeof(msg)){
    pthread_join(data_receiver_thread, NULL);
  }
  log_message("Closing...\n");
  return 0;
}
