#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
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
#include "ipc_utils.h"

//==============Protocol dependent part==================

int ltr_int_send_message(int fifo, uint32_t cmd, uint32_t data)
{
  message_t msg;
  memset(&msg, 0, sizeof(message_t));
  msg.cmd = cmd;
  msg.data = data;
  msg.str[0] = '\0';
  return ltr_int_fifo_send(fifo, &msg, sizeof(message_t));
}

int ltr_int_send_message_w_str(int fifo, uint32_t cmd, uint32_t data, char *str)
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
  return ltr_int_fifo_send(fifo, &msg, sizeof(message_t));
}

int ltr_int_send_data(int fifo, const linuxtrack_full_pose_t *data)
{
  message_t msg;
  memset(&msg, 0, sizeof(message_t));
  msg.cmd = CMD_POSE;
  msg.data = 0;
  msg.pose = *data;
  return ltr_int_fifo_send(fifo, &msg, sizeof(message_t));
}

int ltr_int_send_param_update(int fifo, uint32_t axis, uint32_t param, float value)
{
  message_t msg;
  msg.cmd = CMD_PARAM;
  msg.data = 0; 
  msg.param.axis_id = axis;
  msg.param.param_id = param;
  msg.param.flt_val = value;
  return ltr_int_fifo_send(fifo, &msg, sizeof(message_t));
}

const char *ltr_int_master_fifo_name()
{
  static const char main_fifo[] = "/tmp/ltr_fifi";
  return main_fifo;
}

const char *ltr_int_slave_fifo_name()
{
  static const char slave_fifo[] = "/tmp/ltr_sfifi%02d";
  return slave_fifo;
}

int ltr_int_max_slave_fifos()
{
  return 99;
}
