#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "ltcomm.h"

#define NET_MAX_RETRIES 20

int ltclient_send_msg(ltcomm_net_ctrl_blk_t       *pnet,
                      ltcomm_client_request_msg_t *msg)
{
  printf("sending msg, id=%d\tHost=%s\tPort=%d\n", msg->id,
         inet_ntoa(pnet->server_addr.sin_addr),
         ntohs(pnet->server_addr.sin_port));

  if (sendto(pnet->sd, (void *) msg, sizeof(ltcomm_client_request_msg_t), 0,
             (struct sockaddr *) &(pnet->server_addr),
             sizeof(pnet->server_addr)) == -1) {
    printf("Network sendto failed: id=%d, \n", msg->id);
    printf("errno: %d\n",errno);
    printf("read error: %s\n",strerror(errno));
    return -1;
  }
  return 0;
}

int ltclient_get_reply(ltcomm_net_ctrl_blk_t     *pnet,
                       ltcomm_server_reply_msg_t *msg)
{
  int client_slen = sizeof(struct sockaddr_in);

  if (recvfrom(pnet->sd, (void *) msg, sizeof(ltcomm_server_reply_msg_t), 0,
               (struct sockaddr *) &(pnet->client_addr),
               (socklen_t * )&client_slen) == -1) {
    printf("Network recvfrom failed\n");
    printf("errno: %d\n",errno);
    printf("read error: %s\n",strerror(errno));
    return -1;
  }
  return 0;
}

int ltclient_send_msg_and_get_reply(ltcomm_net_ctrl_blk_t       *pnet,
                                    ltcomm_client_request_msg_t *txmsg,
                                    ltcomm_server_reply_msg_t   *rxmsg)
{
  if (ltclient_send_msg(pnet, txmsg) == -1) {
    return -1;
  }
  if (ltclient_get_reply(pnet, rxmsg) == -1) {
    return -1;
  }
  return 0;
}

int ltclient_send_msg_with_retries(ltcomm_net_ctrl_blk_t       *pnet,
                                   ltcomm_client_request_msg_t *txmsg,
                                   ltcomm_server_reply_msg_t   *rxmsg,
                                   uint8_t                      expected_id)
{
  uint16_t tries = 0;

  while (tries < NET_MAX_RETRIES) {
    if (ltclient_send_msg_and_get_reply(pnet, txmsg, rxmsg) == -1) {
      return -1;
    }
    else if (rxmsg->id == expected_id) {
      return 0;
    }
    else {
      printf("retry: expected=%d\trxed=%d\n",expected_id,rxmsg->id);
      tries++;
    }
  }
  return -1;
}

void sendhelper(ltcomm_net_ctrl_blk_t       *pnet,
                ltcomm_client_request_msg_t *txmsg,
                ltcomm_server_reply_msg_t   *rxmsg,
                uint8_t                      expected_id)
{
  char s[64];
  LTCOMM_CID2STR(txmsg->id,s)
    printf("********************\nsending msg w/ ID=%s\n", s);
  if (ltclient_send_msg_with_retries(pnet,txmsg,rxmsg,expected_id) == -1){
    printf("failed to send msg w/ ID=%s\n", s);
    exit(1);
  }
  printf("received expected reponse to msg w/ ID=%s\n", s);
}

int main(void)
{
  ltcomm_net_ctrl_blk_t       net;
  ltcomm_client_request_msg_t txmsg;
  ltcomm_server_reply_msg_t   rxmsg;

  /*****************************************************************************/
  /* setup network */
  /*****************************************************************************/
  if ((net.sd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    printf("Failed to create socket descriptor\n");
    printf("errno: %d\n",errno);
    printf("Socket error: %s\n",strerror(errno));
  }

  memset(&(net.server_addr), 0, /* Zero the address struct first */
         sizeof((net.server_addr)));
  net.server_addr.sin_family      = AF_INET;
  net.server_addr.sin_addr.s_addr = inet_addr(LTCOMM_DEFAULT_HOST);
  net.server_addr.sin_port        = htons(LTCOMM_DEFAULT_SERVER_PORT);

  memset(&(net.client_addr), 0, /* Zero the address struct first */
         sizeof((net.client_addr)));
  /* bind any port */
  net.client_addr.sin_family           = AF_INET;
  net.client_addr.sin_addr.s_addr      = inet_addr(LTCOMM_DEFAULT_HOST);
  net.client_addr.sin_port             = htons(LTCOMM_DEFAULT_CLIENT_PORT);
  if (bind(net.sd,
           (struct sockaddr *) &(net.client_addr),
           sizeof((net.client_addr))) == -1) {
    printf("Failed to bind to the socket specified descriptor\n");
    printf("errno: %d\n",errno);
    printf("Socket error: %s\n",strerror(errno));
    return -1;
  }

  /*****************************************************************************/
  /* send setup packets */
  /*****************************************************************************/

  txmsg.id = LTCOMM_CID_CONNECT;
  sendhelper(&net, &txmsg, &rxmsg, LTCOMM_SID_ACK);

  txmsg.id = LTCOMM_CID_QUERY_VERSION;
  sendhelper(&net, &txmsg, &rxmsg, LTCOMM_SID_VERSION_REPLY);

  txmsg.id = LTCOMM_CID_START_DATA;
  sendhelper(&net, &txmsg, &rxmsg, LTCOMM_SID_ACK);

  txmsg.id = LTCOMM_CID_REGISTER_PROGRAM_PROFILE_ID;
  sendhelper(&net, &txmsg, &rxmsg, LTCOMM_SID_ACK);

  /*****************************************************************************/
  /* mainloop */
  /*****************************************************************************/
  int i;
  for (i=0; i<5; i++) {
    printf("********************\nsending get data\n");
    txmsg.id                                                 = LTCOMM_CID_GET_DATA;
    if (ltclient_send_msg_and_get_reply(&net,&txmsg,&rxmsg) != -1) {
      if (rxmsg.id == LTCOMM_SID_DATA_REPLY) {
        printf("received get data reply:\n");
        printf("  heading: %f\tpitch: %f\troll: %f\n",
               rxmsg.body.data_body.heading,
               rxmsg.body.data_body.pitch,
               rxmsg.body.data_body.roll);
      }
    }
    sleep(1);
  }

  /*****************************************************************************/
  /* shutdown */
  /*****************************************************************************/
  txmsg.id = LTCOMM_CID_DISCONNECT;
  sendhelper(&net, &txmsg, &rxmsg, LTCOMM_SID_ACK);

  txmsg.id = LTCOMM_CID_TERMINATE;
  sendhelper(&net, &txmsg, &rxmsg, LTCOMM_SID_ACK);

  return 0;
}

