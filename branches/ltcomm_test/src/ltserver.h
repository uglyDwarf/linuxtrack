#ifndef LTSERVER__H
#define LTSERVER__H 1
#include "ltcomm.h"
#include "ltlib.h"
#include <stdbool.h>
/* for joystick */
#include <linux/joystick.h>

#define LTSERVER_VERSION 0x400

#define LTSERVER_CONFIG_SECTION                  "Ltserver"
#define LTSERVER_CONFIG_HOSTNAME_PREFKEY         "server-hostname"
#define LTSERVER_CONFIG_SERVERPORT_PREFKEY       "server-port"
#define LTSERVER_CONFIG_RECENTER_DEVICE_PREFKEY  "recenter-joy-device"
#define LTSERVER_CONFIG_RECENTER_EVENT_PREFKEY   "recenter-joy-event-type"
#define LTSERVER_CONFIG_RECENTER_NUMBER_PREFKEY  "recenter-joy-event-number"
#define LTSERVER_CONFIG_RECENTER_VALUE_PREFKEY   "recenter-joy-event-value"
#define LTSERVER_DEFAULT_JOY_DEV                 "/dev/input/js0"
#define LTSERVER_DEFAULT_JOY_RECENTER_EVENT_TYPE JS_EVENT_BUTTON
#define LTSERVER_DEFAULT_JOY_RECENTER_NUMBER     1
#define LTSERVER_DEFAULT_JOY_RECENTER_VALUE      1

#define LTSERVER_JOY_EVENT_FLUSHDEPTH 100

typedef struct {
  int              fd;
  char            *device_name;
  struct js_event  recenter_event;
} ltserver_joy_ctrl_blk_t;

int ltserver_init_linuxtrack(void);

int ltserver_init_joystick(ltserver_joy_ctrl_blk_t *pjoy);

int ltserver_init_network(ltcomm_net_ctrl_blk_t *pnet);

int ltserver_get_network_message(ltcomm_net_ctrl_blk_t       *pnet,
                                 ltcomm_client_request_msg_t *msg);

int ltserver_check_for_recenter_request(ltserver_joy_ctrl_blk_t *pjoy,
                                        bool                    *recenter_requested);

int ltserver_update_linuxtrack(void);


#endif
