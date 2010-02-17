#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include "netcomm.h"
#include "utils.h"

pthread_t data_receiver_thread;

void *data_receiver(void *arg)
{
  struct blob_type nbt[3];
  struct bloblist_type nb;
  nb.blobs = nbt;
  int cfd = *(int *)arg;
  printf("Data receiver starting!\n");
  ssize_t ret;
  unsigned char msg[1024];
  while(1){
    ret = read(cfd, &msg, sizeof(msg));
    if(ret < 0){
      if(errno == EINTR){
        continue;
      }else{
        perror("Read");
        break;
      }
    }
    if(ret == 0){
      log_message("Server disconnected!\n");
      break;
    }
    if(ret > 0){
      decode_bloblist(&nb, msg);
      
      printf("%d blobs!\n", nb.num_blobs);
      printf("[%g, %g]\n", (nb.blobs[0]).x, (nb.blobs[0]).y);
      printf("[%g, %g]\n", (nb.blobs[1]).x, (nb.blobs[1]).y);
      printf("[%g, %g]\n\n", (nb.blobs[2]).x, (nb.blobs[2]).y);
    }
  }
  printf("Data receiver stopping!\n");
  return NULL;
}

int client(int sfd)
{
  unsigned char msg;
  
  sleep(1);
  msg = RUN;
  write(sfd, &msg, sizeof(msg));
  printf("RUN\n");
  sleep(10);
  msg = SUSPEND;
  write(sfd, &msg, sizeof(msg));
  printf("SUSPEND\n");
  sleep(5);
  msg = WAKE;
  write(sfd, &msg, sizeof(msg));
  printf("WAKE\n");
  sleep(2);
  msg = SHUTDOWN;
  write(sfd, &msg, sizeof(msg));
  printf("SHUTDOWN\n");
  sleep(1);
  
  close(sfd);
  
  return 0;
}


int main(int argc, char *argv[])
{
/*  struct blob_type bt[] = {{.x = 111, .y = 222, .score = 13}, 
              {.x = 333, .y = 444, .score = 113},
	      {.x = 555, .y = 6666, .score = 123}
	      };
  struct blob_type nbt[3];
  struct bloblist_type blobs = {
    .num_blobs = 3,
    .blobs = bt};
  struct bloblist_type nb;
  nb.blobs = nbt;
  char buffer[2048];
  
  printf("Buf: %d bytes\n", encode_bloblist(&blobs, buffer));
  decode_bloblist(&nb, buffer);
  
  printf("%d blobs!\n", nb.num_blobs);
  printf("[%g, %g]\n", (nb.blobs[0]).x, (nb.blobs[0]).y);
  printf("[%g, %g]\n", (nb.blobs[1]).x, (nb.blobs[1]).y);
  printf("[%g, %g]\n", (nb.blobs[2]).x, (nb.blobs[2]).y);
*/  
  if(argc != 3){
    fprintf(stderr, "Bad args...\n");
    return 1;
  }
  int sfd;
  if((sfd = init_client(argv[1], atoi(argv[2]), 3)) < 0){
    fprintf(stderr, "Have problem....\n");
    return 1;
  }
  pthread_create(&data_receiver_thread, NULL, data_receiver, &sfd);
  client(sfd);
  close(sfd);
  printf("Seems to work...\n");
  return 0;
}
