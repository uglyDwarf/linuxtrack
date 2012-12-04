#ifndef LTR_SRV_COMM__H
#define LTR_SRV_COMM__H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <linuxtrack.h>
#include <ltlib.h>

typedef struct{
  uint32_t axis_id;
  uint32_t param_id;
  union{
    float flt_val;
    bool bool_val;
  };
} param_t;

typedef struct{
  uint32_t cmd;
  uint32_t data;
  union{
    char str[500];
    pose_t pose;
    param_t param;
  };
} message_t;

enum cmds {CMD_NOP, CMD_NEW_FIFO, CMD_PAUSE, CMD_WAKEUP, CMD_RECENTER, CMD_POSE, CMD_PARAM};

#ifdef __cplusplus
extern "C" {
#endif

bool make_fifo(const char *name);
int open_fifo_exclusive(const char *name);
int open_fifo_for_writing(const char *name);
int open_unique_fifo(char **name, int *num, const char *name_template, int max);
int fifo_send(int fifo, void *buf, size_t size);
int fifo_receive(int fifo, void *buf, size_t size);

int send_message(int fifo, uint32_t cmd, uint32_t data);
int send_message_w_str(int fifo, uint32_t cmd, uint32_t data, char *str);
int send_data(int fifo, const pose_t *data);
int send_param_update(int fifo, uint32_t axis, uint32_t param, float value);
const char *master_fifo_name();
const char *slave_fifo_name();
int max_slave_fifos();

#ifdef __cplusplus
}
#endif

#endif

