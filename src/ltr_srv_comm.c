#define _GNU_SOURCE
#include <stdio.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <string.h>

#include "ltr_srv_comm.h"

bool make_fifo(const char *name)
{
  if(name == NULL){
    printf("Name must be set! (NULL passed)\n");
  }
  struct stat info;
  if(stat(name, &info) == 0){
    //file exists, check if it is fifo...
    if(S_ISFIFO(info.st_mode)){
      printf("Fifo exists already!\n");
      return true;
    }else if(S_ISDIR(info.st_mode)){
      printf("Directory exists! Will try to remove it.\n");
      if(rmdir(name) != 0){
        perror("rmdir");
        return false;
      }
    }else{
      printf("File exists, but it is not a fifo! Will try to remove it.\n");
      if(unlink(name) != 0){
        perror("unlink");
        return false;
      }
    }
  }
  //At this point, the file should not exist.
  if(mkfifo(name, S_IRUSR | S_IWUSR) != 0){
    perror("mkfifo");
    return false;
  }
  printf("Fifo created!\n");
  return true;
}

int open_fifo_exclusive(const char *name)
{
  if(!make_fifo(name)){
    return -1;
  }
  int fifo = -1;
  if((fifo = open(name, O_RDONLY | O_NONBLOCK)) > 0){
    printf("Fifo %s opened!\n", name);
    if(flock(fifo, LOCK_EX | LOCK_NB) == 0){
      printf("Fifo %s (fd %d) opened and locked!\n", name, fifo);
      return fifo;
    }
    printf("Fifo %s (fd %d) could not be locked, closing it!\n", name, fifo);
    close(fifo);
  }
  return -1;
}

int open_fifo_for_writing(const char *name){
  if(!make_fifo(name)){
    return -1;
  }
  
  //Open the pipe (handling the wait for the other party to open reading end)
  //  Todo: add some timeout?
  int fifo = -1;
  int timeout = 3;
  while(timeout > 0){
    --timeout;
    if((fifo = open(name, O_WRONLY | O_NONBLOCK)) < 0){
      if(errno != ENXIO){
        perror("open@open_fifo_for_writing");
        return -1;
      }
      fifo = -1;
      sleep(1);
    }else{
      break;
    }
  }
  return fifo;
}

int open_unique_fifo(char **name, int *num, const char *template, int max)
{
  //although I could come up with method allowing more than 100
  //  fifos to be checked, there is not a point doing so
  int i;
  char *fifo_name = NULL;
  int fifo = -1;
  for(i = 0; i < max; ++i){
    asprintf(&fifo_name, template, i);
    fifo = open_fifo_exclusive(fifo_name);
    if(fifo > 0){
      break;
    }
    free(fifo_name);
    fifo_name = NULL;
  }
  *name = fifo_name;
  *num = i;
  return fifo;
}

int fifo_send(int fifo, void *buf, size_t size)
{
  if(fifo <= 0){
    return -1;
  }
  if(write(fifo, buf, size) < 0){
    printf("Write @fd %d failed:\n", fifo);
    perror("write@pipe_send");
    return errno;
   }
  return 0;
}

int fifo_receive(int fifo, void *buf, size_t size)
{
  //Assumption is, that write/read of less than 512 bytes should be atomic...
  ssize_t num_read = read(fifo, buf, size);
  if(num_read > 0){
    return 0;
  }else if(num_read < 0){
    perror("read@fifo_receive");
    return errno;
  }
  return 0;
}

//==============Protocol dependent part==================

int send_message(int fifo, uint32_t cmd, uint32_t data)
{
  message_t msg;
  memset(&msg, 0, sizeof(message_t));
  msg.cmd = cmd;
  msg.data = data;
  msg.str[0] = '\0';
  return fifo_send(fifo, &msg, sizeof(message_t));
}

int send_message_w_str(int fifo, uint32_t cmd, uint32_t data, char *str)
{
  message_t msg;
  memset(&msg, 0, sizeof(message_t));
  msg.cmd = cmd;
  msg.data = data;
  if(str != NULL){
    strncpy(&(msg.str[0]), str, 500);
    msg.str[499] = '\0'; //in case the str is longer than 500 bytes, 
                         //  the copy would not be null terminated!
  }else{
    msg.str[0] = '\0';
  }
  printf("Sending string %s\n", msg.str);
  return fifo_send(fifo, &msg, sizeof(message_t));
}

int send_data(int fifo, const pose_t *data)
{
  message_t msg;
  memset(&msg, 0, sizeof(message_t));
  msg.cmd = CMD_POSE;
  msg.data = 0;
  msg.pose = *data;
  return fifo_send(fifo, &msg, sizeof(message_t));
}

int send_param_update(int fifo, uint32_t axis, uint32_t param, float value)
{
  message_t msg;
  msg.cmd = CMD_PARAM;
  msg.data = 0; 
  msg.param.axis_id = axis;
  msg.param.param_id = param;
  msg.param.flt_val = value;
  return fifo_send(fifo, &msg, sizeof(message_t));
}

const char *master_fifo_name()
{
  static const char main_fifo[] = "/tmp/ltr_fifi";
  return main_fifo;
}

const char *slave_fifo_name()
{
  static const char slave_fifo[] = "/tmp/ltr_sfifi%02d";
  return slave_fifo;
}

int max_slave_fifos()
{
  return 99;
}
