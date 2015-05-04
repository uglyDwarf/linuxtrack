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
        ltr_int_my_perror("select()");
        exit(EXIT_FAILURE);
    }
    if (FD_ISSET(cfd, &rd)) {
     repeat:
      ret = read(cfd, &msg, sizeof(msg));
      if(ret < 0){
        if(errno == EINTR){
          goto repeat;
        }else{
          ltr_int_my_perror("read");
          break;
        }
      }
      if(ret == 0){
        break;
      }
      if(ret > 0){
/*        char txt[4096];
        char *txtp = txt;
        for(r=0; r<ret; ++r){
          sprintf(txtp, "%02X ", msg[r]);
          txtp+=3;
        }
        sprintf(txtp, "\n\n");
        log_message("%s", txt);*/
        decode_bloblist(&(frame.bloblist), msg);
        update_pose(&frame);
        /*log_message("%d blobs!\n", frame.bloblist.num_blobs);
        log_message("[%g, %g]\n", (frame.bloblist.blobs[0]).x, (frame.bloblist.blobs[0]).y);
        log_message("[%g, %g]\n", (frame.bloblist.blobs[1]).x, (frame.bloblist.blobs[1]).y);
        log_message("[%g, %g]\n\n", (frame.bloblist.blobs[2]).x, (frame.bloblist.blobs[2]).y);
        */
      }
    }
    
  }
  close(cfd);
  cfd = -1;
  return NULL;
}

int send_msg(int cfd, message_t msg)
{
 repeat:
  if(write(cfd, &msg, sizeof(msg)) < 0){
    if(errno == EINTR){
      goto repeat;
    }else{
      ltr_int_my_perror("write");
      return -1;
    }
  }
  return 0;
}

int lt_client_init()
{
  int sfd;
  if((sfd = init_client("localhost", 3004, 3)) < 0){ //TODO
    log_message("Can't initialize client!\n");
    return 1;
  }
  close_receiver = false;
  pthread_create(&data_receiver_thread, NULL, lt_data_receiver, &sfd);
  log_message("Initializing...\n");
  return send_msg(cfd, RUN);
}

int lt_client_suspend()
{
  return send_msg(cfd, SUSPEND);
}

int lt_client_wakeup()
{
  return send_msg(cfd, WAKE);
}

int lt_client_close()
{
  close_receiver = true;
  return send_msg(cfd, SHUTDOWN);
  message_t msg;
  msg = SHUTDOWN;
  if(write(cfd, &msg, sizeof(msg)) == sizeof(msg)){
    pthread_join(data_receiver_thread, NULL);
    pthread_detach(data_receiver_thread);
  }
  log_message("Closing...\n");
  return 0;
}
