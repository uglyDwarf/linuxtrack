#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "netcomm.h"

pthread_t data_receiver_thread;

void *data_receiver(void *arg)
{
  int cfd = *(int *)arg;
  printf("Data receiver starting!\n");
  ssize_t ret;
  char msg[1024];
  while(1){
    ret = read(cfd, &msg, sizeof(msg));
    if(ret > 0){
      printf("%s\n", msg);
    }
  }
  
  printf("Data receiver stopping!\n");
}

int client(int sfd)
{
  unsigned char msg;
  
  sleep(1);
  msg = INIT;
  write(sfd, &msg, sizeof(msg));
  sleep(10);
  msg = WAKE;
  write(sfd, &msg, sizeof(msg));
  sleep(10);
  msg = SUSPEND;
  write(sfd, &msg, sizeof(msg));
  sleep(1);
  msg = WAKE;
  write(sfd, &msg, sizeof(msg));
  sleep(2);
  msg = SHUTDOWN;
  write(sfd, &msg, sizeof(msg));
  sleep(1);
  
  close(sfd);
  
  return 0;
}


int main(int argc, char *argv[])
{
  if(argc != 3){
    fprintf(stderr, "Bad args...\n");
    return 1;
  }
  int sfd;
  if((sfd = init_client(argv[1], atoi(argv[2]))) < 0){
    fprintf(stderr, "Have problem....\n");
    return 1;
  }
  pthread_create(&data_receiver_thread, NULL, data_receiver, &sfd);
  client(sfd);
  close(sfd);
  printf("Seems to work...\n");
  return 0;
}
