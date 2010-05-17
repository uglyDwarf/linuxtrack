#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "ltserver.h"

//#define lt_log_message printf

int ltserver_init_linuxtrack(void)
{
  struct lt_configuration_type ltconf;
  if(lt_init(ltconf, LTSERVER_CONFIG_SECTION) != 0){
    return -1;
  }
  lt_suspend();
  return 0;
}

int ltserver_init_joystick(ltserver_joy_ctrl_blk_t *pjoy)
{
  int  num_of_axis    = 0;
  int  num_of_buttons = 0;
  char name_of_joystick[128];

  printf("INFO: Don't forget to have the bulk config data in the current working dir!\n");

  pref_id joy_recenter_device_pref;
  if ( !lt_open_pref(LTSERVER_CONFIG_RECENTER_DEVICE_PREFKEY,
                     &joy_recenter_device_pref) ) {
    pjoy->device_name = LTSERVER_DEFAULT_JOY_DEV;
    lt_log_message("Unable to find preference key for %s\n",
                   LTSERVER_CONFIG_RECENTER_DEVICE_PREFKEY);
    lt_log_message("Using hardcoded default for pref %s: %s\n",
                   LTSERVER_CONFIG_RECENTER_DEVICE_PREFKEY,
                   pjoy->device_name);
  }
  else {
    pjoy->device_name = lt_get_str(joy_recenter_device_pref);
    lt_log_message("Pref %s found: %s\n",
                   LTSERVER_CONFIG_RECENTER_DEVICE_PREFKEY,
                   pjoy->device_name);
  }

  pref_id joy_recenter_event_pref;
  if ( !lt_open_pref(LTSERVER_CONFIG_RECENTER_EVENT_PREFKEY,
                     &joy_recenter_event_pref) ) {
    pjoy->recenter_event.type = LTSERVER_DEFAULT_JOY_RECENTER_EVENT_TYPE;
    lt_log_message("Unable to find preference key for %s\n",
                   LTSERVER_CONFIG_RECENTER_EVENT_PREFKEY);
    lt_log_message("Using hardcoded default for pref %s: %s\n",
                   LTSERVER_CONFIG_RECENTER_EVENT_PREFKEY,
                   pjoy->recenter_event.type);
  }
  else {
    char *event_type_str        = lt_get_str(joy_recenter_event_pref);
    lt_log_message("Pref %s found: %s\n",
                   LTSERVER_CONFIG_RECENTER_EVENT_PREFKEY,
                   event_type_str);
    if (!strncmp(event_type_str, "AXIS",4)) {
      pjoy->recenter_event.type = JS_EVENT_AXIS;
      lt_log_message("Using JS_EVENT_AXIS for %s\n",
                     LTSERVER_CONFIG_RECENTER_EVENT_PREFKEY);
    }
    else if (!strncmp(event_type_str, "BUTTON",6)) {
      pjoy->recenter_event.type = JS_EVENT_BUTTON;
      lt_log_message("Using JS_EVENT_BUTTON for %s\n",
                     LTSERVER_CONFIG_RECENTER_EVENT_PREFKEY);
    }
    else {
      lt_log_message("Error: %s must be either AXIS or BUTTON, saw: %s\n",
                     LTSERVER_CONFIG_RECENTER_EVENT_PREFKEY,
                     lt_get_str(joy_recenter_event_pref));
      exit(1);
    }
  }

  pref_id joy_recenter_number_pref;
  if ( !lt_open_pref(LTSERVER_CONFIG_RECENTER_NUMBER_PREFKEY,
                     &joy_recenter_number_pref) ) {
    pjoy->recenter_event.number = LTSERVER_DEFAULT_JOY_RECENTER_NUMBER;
    lt_log_message("Unable to find preference key for %s\n",
                   LTSERVER_CONFIG_RECENTER_NUMBER_PREFKEY);
    lt_log_message("Using hardcoded default for pref %s: %d\n",
                   LTSERVER_CONFIG_RECENTER_NUMBER_PREFKEY,
                   pjoy->recenter_event.number);
  }
  else {
    pjoy->recenter_event.number = lt_get_int(joy_recenter_number_pref);
    lt_log_message("Pref %s found: %d\n",
                   LTSERVER_CONFIG_RECENTER_NUMBER_PREFKEY,
                   pjoy->recenter_event.number);
  }
  pref_id joy_recenter_value_pref;
  if ( !lt_open_pref(LTSERVER_CONFIG_RECENTER_VALUE_PREFKEY,
                     &joy_recenter_value_pref) ) {
    pjoy->recenter_event.value = LTSERVER_DEFAULT_JOY_RECENTER_VALUE;
    lt_log_message("Unable to find preference key for %s\n",
                   LTSERVER_CONFIG_RECENTER_VALUE_PREFKEY);
    lt_log_message("Using hardcoded default for pref %s: %d\n",
                   LTSERVER_CONFIG_RECENTER_VALUE_PREFKEY,
                   pjoy->recenter_event.value);
  }
  else {
    pjoy->recenter_event.value = lt_get_int(joy_recenter_value_pref);
    lt_log_message("Pref %s found: %d\n",
                   LTSERVER_CONFIG_RECENTER_VALUE_PREFKEY,
                   pjoy->recenter_event.value);
  }

  if( (pjoy->fd = open(pjoy->device_name, O_RDONLY)) == -1 ) {
    lt_log_message("Couldn't open joystick %s\n",
                   pjoy->device_name);
    return -1;
  }
  if (ioctl( pjoy->fd, JSIOCGAXES, &num_of_axis ) == -1) {
    lt_log_message("Request for number of joystick axes failed\n");
    return -1;
  }
  if (ioctl( pjoy->fd, JSIOCGBUTTONS, &num_of_buttons ) == -1) {
    lt_log_message("Request for number of joystick buttons failed\n");
    return -1;
  }
  if (ioctl( pjoy->fd, JSIOCGNAME(80), &name_of_joystick ) == -1) {
    lt_log_message("Request for name of joystick failed\n");
    return -1;
  }
  if (fcntl( pjoy->fd, F_SETFL, O_NONBLOCK ) == -1) { /* use non-blocking mode */
    lt_log_message("Request to set joystick reads to non-blocking failed\n");
    return -1;
  }
  lt_log_message("Joystick detected: %s\n", name_of_joystick);
  lt_log_message("Joystick detected: %d axes\n", num_of_axis);
  lt_log_message("Joystick detected: %d buttons\n", num_of_buttons);
  return 0;
}

int ltserver_init_network(ltserver_net_ctrl_blk_t *pnet)
{
  pref_id  net_hostname_pref;
  char    *hostname;
  if ( !lt_open_pref(LTSERVER_CONFIG_HOSTNAME_PREFKEY,
                     &net_hostname_pref) ) {
    hostname = LTCOMM_DEFAULT_HOST;
    lt_log_message("Unable to find preference key for %s\n",
                   LTSERVER_CONFIG_HOSTNAME_PREFKEY);
    lt_log_message("Using hardcoded default for pref %s: %s\n",
                   LTSERVER_CONFIG_HOSTNAME_PREFKEY,
                   hostname);
  }
  else {
    hostname = lt_get_str(net_hostname_pref);
    lt_log_message("Pref %s found: %s\n",
                   LTSERVER_CONFIG_HOSTNAME_PREFKEY,
                   hostname);
  }

  pref_id  net_server_port_pref;
  uint16_t server_port;
  if ( !lt_open_pref(LTSERVER_CONFIG_SERVERPORT_PREFKEY,
                     &net_server_port_pref) ) {
    server_port = LTCOMM_DEFAULT_SERVER_PORT;
    lt_log_message("Unable to find preference key for %s\n",
                   LTSERVER_CONFIG_SERVERPORT_PREFKEY);
    lt_log_message("Using hardcoded default for pref %s: %d\n",
                   LTSERVER_CONFIG_SERVERPORT_PREFKEY,
                   server_port);
  }
  else {
    server_port = lt_get_int(net_server_port_pref);
    lt_log_message("Pref %s found: %d\n",
                   LTSERVER_CONFIG_SERVERPORT_PREFKEY,
                   server_port);
  }

  if ((pnet->sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    lt_log_message("Failed to create server socket descriptor\n");
    lt_log_message("errno: %d\n",errno);
    lt_log_message("Socket error: %s\n",strerror(errno));
    return -1;
  }

  memset(&(pnet->server_addr), 0, /* Zero the address struct first */
         sizeof((pnet->server_addr)));
  (pnet->server_addr).sin_family      = AF_INET;
  (pnet->server_addr).sin_addr.s_addr = inet_addr(hostname);
  (pnet->server_addr).sin_port        = htons(server_port);

  /* bind the socket to the port number */
  if (bind(pnet->sd,
           (struct sockaddr *) &(pnet->server_addr),
           sizeof((pnet->server_addr))) == -1) {
    lt_log_message("Failed to bind to the socket specified descriptor\n");
    lt_log_message("errno: %d\n",errno);
    lt_log_message("Socket error: %s\n",strerror(errno));
    return -1;
  }
  return 0;
}

int ltserver_check_for_recenter_request(ltserver_joy_ctrl_blk_t *pjoy,
                                        bool                    *recenter_requested)
{
  struct js_event js;
  int             i = 0;

  *recenter_requested = false;

  // basically, flush out events till we hit the max reads,
  //   no data is read, or we hit our target event
  // the intent is to flush all the non-recenter events
  while (i<LTSERVER_JOY_EVENT_FLUSHDEPTH){
    if (read(pjoy->fd, &js, sizeof(struct js_event)) == -1) {
      if (errno == EAGAIN) {
        // this just means no data read, drop out early
        return 0;
      }
      else {
        lt_log_message("Request to read joystick failed\n");
        lt_log_message("errno: %d\n",errno);
        lt_log_message("read error: %s\n",strerror(errno));
        return -1;
      }
    }
    //    printf("event type: %d\tevent number: %d\tevent value: %d\n",(js.type & ~JS_EVENT_INIT),js.number, js.value);
    if (((js.type & ~JS_EVENT_INIT) == pjoy->recenter_event.type) &&
        (js.number == pjoy->recenter_event.number)                &&
        (js.value == pjoy->recenter_event.value)) {
      *recenter_requested            = true;
      // found it, drop out early
      return 0;
    }
    i++;
  }
  return 0;
}

int ltserver_send_msg(ltserver_net_ctrl_blk_t   *pnet,
                      npcomm_server_reply_msg_t *msg)
{
  if (sendto(pnet->sd, (void *) msg, sizeof(npcomm_server_reply_msg_t), 0,
             (struct sockaddr *) &(pnet->client_addr),
             sizeof(pnet->client_addr)) == -1) {
    lt_log_message("Network sendto failed: id=%d, \n", msg->id);
    lt_log_message("errno: %d\n",errno);
    lt_log_message("read error: %s\n",strerror(errno));
    return -1;
  }
  return 0;
}

int ltserver_send_nack(ltserver_net_ctrl_blk_t *pnet,
                       uint16_t                 err)
{
  npcomm_server_reply_msg_t msg;
  msg.id                = LTCOMM_SID_NACK;
  msg.body.nondata_body = err;
  return ltserver_send_msg(pnet, &msg);
}

int ltserver_send_ack(ltserver_net_ctrl_blk_t *pnet)
{
  npcomm_server_reply_msg_t msg;
  msg.id                = LTCOMM_SID_ACK;
  msg.body.nondata_body = 0;
  return ltserver_send_msg(pnet, &msg);
}

int ltserver_send_data_reply(ltserver_net_ctrl_blk_t  *pnet,
                             npcomm_get_data_coords_t  data)
{
  npcomm_server_reply_msg_t msg;
  msg.id                 = LTCOMM_SID_DATA_REPLY;
  msg.body.get_data_body = data;
  return ltserver_send_msg(pnet, &msg);
}

int ltserver_send_version_reply(ltserver_net_ctrl_blk_t *pnet,
                                uint16_t                 version)
{
  npcomm_server_reply_msg_t msg;
  msg.id                = LTCOMM_SID_VERSION_REPLY;
  msg.body.nondata_body = version;
  return ltserver_send_msg(pnet, &msg);
}

int ltserver_send_param_reply(ltserver_net_ctrl_blk_t *pnet,
                              uint16_t                 param)
{
  npcomm_server_reply_msg_t msg;
  msg.id                = LTCOMM_SID_PARAM_REPLY;
  msg.body.nondata_body = param;
  return ltserver_send_msg(pnet, &msg);
}

int ltserver_get_network_message(ltserver_net_ctrl_blk_t     *pnet,
                                 npcomm_client_request_msg_t *msg)
{
  int  client_slen   = sizeof(struct sockaddr_in);
  bool got_valid_msg = false;

  while (!got_valid_msg) {
    if (recvfrom(pnet->sd, msg, sizeof(npcomm_client_request_msg_t), 0,
                 (struct sockaddr *)&(pnet->client_addr),
                 (socklen_t * )&client_slen) == -1) {
      lt_log_message("Network recvfrom failed\n");
      lt_log_message("errno: %d\n",errno);
      lt_log_message("read error: %s\n",strerror(errno));
      return -1;
    }
    switch (msg->id) {
    case LTCOMM_CID_CONNECT:
    case LTCOMM_CID_QUERY_VERSION:
    case LTCOMM_CID_GET_PARAM:
    case LTCOMM_CID_START_DATA:
    case LTCOMM_CID_REGISTER_PROGRAM_PROFILE_ID:
    case LTCOMM_CID_STOP_DATA:
    case LTCOMM_CID_RECENTER:
    case LTCOMM_CID_GET_DATA:
    case LTCOMM_CID_DISCONNECT:
    case LTCOMM_CID_TERMINATE:
      got_valid_msg = true;
      break;
    default:
      ltserver_send_nack(pnet, LTCOMM_ERR_INVALID_MSG);
      return -1;
    }
  }
  return 0;
}

int main(void)
{
  /*****************************************************************************/
  /*** SETUP *******************************************************************/
  /*****************************************************************************/

  /*** Linux-track *************************************************************/
  if (ltserver_init_linuxtrack() != 0) {
    lt_log_message("Unable to initialize linuxtrack!\n");
    exit(1);
  }
  else {
    lt_log_message("Successfully initialized linuxtrack\n");
  }

  /*** network *****************************************************************/
  ltserver_net_ctrl_blk_t net;
  if (ltserver_init_network(&net) != 0) {
    lt_log_message("Unable to initialize network!\n");
    exit(1);
  }
  else {
    lt_log_message("Successfully initialized network\n");
  }

  /*** joystick ****************************************************************/
  ltserver_joy_ctrl_blk_t joy;
  if (ltserver_init_joystick(&joy) != 0) {
    lt_log_message("Unable to initialize joystick! No local recenter key support\n");
  }
  else {
    lt_log_message("Successfully initialized joystick\n");
  }

  /*****************************************************************************/
  /*** MAINLOOP ****************************************************************/
  /*****************************************************************************/
  bool                        terminate          = false;
  bool                        running            = false;
  bool                        recenter_requested = false;
  npcomm_client_request_msg_t cmsg;
  int                         ltretval;
  npcomm_get_data_coords_t    dc;

  while (!terminate) {
    /* network update */
    if (ltserver_get_network_message(&net,&cmsg) == -1) {
      lt_log_message("ltserver_get_network_message failed\n");
    }
    else {
      switch (cmsg.id) {
      case LTCOMM_CID_CONNECT:
        lt_log_message("Connection: Host: %s\tPort: %d\n",
                       inet_ntoa(net.client_addr.sin_addr),
                       ntohs(net.client_addr.sin_port));
        ltserver_send_ack(&net);
        break;
      case LTCOMM_CID_QUERY_VERSION:
        ltserver_send_version_reply(&net, LTSERVER_VERSION);
        break;
      case LTCOMM_CID_GET_PARAM:
        ltserver_send_param_reply(&net, 0);
        break;
      case LTCOMM_CID_START_DATA:
        lt_wakeup();
        ltserver_send_ack(&net);
        running  = true;
        break;
      case LTCOMM_CID_REGISTER_PROGRAM_PROFILE_ID:
        /* fixme! do something here! */
        ltserver_send_ack(&net);
        break;
      case LTCOMM_CID_STOP_DATA:
        lt_suspend();
        ltserver_send_ack(&net);
        running  = false;
        break;
      case LTCOMM_CID_RECENTER:
        lt_recenter();
        ltserver_send_ack(&net);
        break;
      case LTCOMM_CID_GET_DATA:
        ltretval = lt_get_camera_update(&dc.heading,&dc.pitch,&dc.roll,
                                        &dc.tx, &dc.ty, &dc.tz);
        if(ltretval < 0) {
          lt_log_message("Error code %d detected!\n", ltretval);
          lt_shutdown();
          exit(1);
        }
        ltserver_send_data_reply(&net, dc);
        break;
      case LTCOMM_CID_DISCONNECT:
        /* anything else to do here? */
        lt_log_message("Disconnect: Host: %s\tPort: %d\n",
                       inet_ntoa(net.client_addr.sin_addr),
                       ntohs(net.client_addr.sin_port));
        lt_suspend();
        ltserver_send_ack(&net);
        break;
      case LTCOMM_CID_TERMINATE:
        ltserver_send_ack(&net);
        terminate = true;
        break;
      }
      /* joystick update */
      if (running) {
        if (ltserver_check_for_recenter_request(&joy,&recenter_requested) != -1) {
          if (recenter_requested) {
            lt_recenter();
          }
        }
      }
    }
  }
  lt_shutdown();
  return 0;
}
