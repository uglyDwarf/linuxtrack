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
    linuxtrack_full_pose_t pose;
    param_t param;
  };
} message_t;

enum cmds {CMD_NOP, CMD_NEW_SOCKET, CMD_PAUSE, CMD_WAKEUP, CMD_RECENTER, CMD_POSE, CMD_PARAM,
           CMD_FRAMES};

#ifdef __cplusplus
extern "C" {
#endif

int ltr_int_send_message(int socket, uint32_t cmd, uint32_t data);
int ltr_int_send_message_w_str(int socket, uint32_t cmd, uint32_t data, char *str);
int ltr_int_send_data(int socket, const linuxtrack_full_pose_t *data);
int ltr_int_send_param_update(int socket, uint32_t axis, uint32_t param, float value);
const char *ltr_int_master_socket_name();
const char *ltr_int_slave_socket_name();
int ltr_int_max_slave_sockets();

#ifdef __cplusplus
}
#endif

#endif

