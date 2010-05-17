#include <stdio.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "ltcomm.h"


int main(void)
{
  int                         sd;
  struct  sockaddr_in         server_addr;
  struct  sockaddr_in         client_addr;
  npcomm_client_request_msg_t txmsg;
  npcomm_server_reply_msg_t   rxmsg;

  /*****************************************************************************/
  /* setup network */
  /*****************************************************************************/
  if ((sd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    printf("Failed to create socket descriptor\n");
    printf("errno: %d\n",errno);
    printf("Socket error: %s\n",strerror(errno));
  }

  memset(&(server_addr), 0, /* Zero the address struct first */
         sizeof((server_addr)));
  server_addr.sin_family      = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(LTCOMM_DEFAULT_HOST);
  server_addr.sin_port        = htons(LTCOMM_DEFAULT_SERVER_PORT);

  memset(&(client_addr), 0, /* Zero the address struct first */
         sizeof((client_addr)));
  /* bind any port */
  client_addr.sin_family           = AF_INET;
  client_addr.sin_addr.s_addr      = inet_addr(LTCOMM_DEFAULT_HOST);
  client_addr.sin_port             = htons(LTCOMM_DEFAULT_CLIENT_PORT);
  if (bind(sd,
           (struct sockaddr *) &(client_addr),
           sizeof((client_addr))) == -1) {
    printf("Failed to bind to the socket specified descriptor\n");
    printf("errno: %d\n",errno);
    printf("Socket error: %s\n",strerror(errno));
    return -1;
  }

  /*****************************************************************************/
  /* send setup packets */
  /*****************************************************************************/
  bool acked = false;

  while (!acked) {
    txmsg.id                         = LTCOMM_CID_CONNECT;
    printf("sending connect request\n");
    if (sendto(sd, (void *) &txmsg, sizeof(npcomm_client_request_msg_t), 0,
               (struct sockaddr *) &server_addr,
               sizeof(server_addr)) == -1) {
      printf("Network sendto failed: id=%d\n", txmsg.id);
      printf("errno: %d\n",errno);
      printf("read error: %s\n",strerror(errno));
      return -1;
    }
    rxmsg.id                                  = LTCOMM_SID_ACK;
    int client_slen                           = sizeof(struct sockaddr_in);
    if (recvfrom(sd, (void *) &rxmsg, sizeof(npcomm_server_reply_msg_t), 0,
                 (struct sockaddr *)&(client_addr),
                 (socklen_t * )&client_slen) == -1) {
      printf("Network recvfrom failed\n");
      printf("errno: %d\n",errno);
      printf("read error: %s\n",strerror(errno));
      return -1;
    }
    else {
      if (rxmsg.id == LTCOMM_SID_ACK) {
        acked = true;
        printf("received connect ack\n");
      }
    }
  }

  acked                              = false;
  while (!acked) {
    txmsg.id                         = LTCOMM_CID_QUERY_VERSION;
    printf("sending query version\n");
    if (sendto(sd, (void *) &txmsg, sizeof(npcomm_client_request_msg_t), 0,
               (struct sockaddr *) &server_addr,
               sizeof(server_addr)) == -1) {
      printf("Network sendto failed: id=%d\n", txmsg.id);
      printf("errno: %d\n",errno);
      printf("read error: %s\n",strerror(errno));
      return -1;
    }
    rxmsg.id                                  = LTCOMM_SID_ACK;
    int client_slen                           = sizeof(struct sockaddr_in);
    if (recvfrom(sd, (void *) &rxmsg, sizeof(npcomm_server_reply_msg_t), 0,
                 (struct sockaddr *)&(client_addr),
                 (socklen_t * )&client_slen) == -1) {
      printf("Network recvfrom failed\n");
      printf("errno: %d\n",errno);
      printf("read error: %s\n",strerror(errno));
      return -1;
    }
    else {
      if (rxmsg.id == LTCOMM_SID_VERSION_REPLY) {
        acked = true;
        printf("received version reply = %x\n", rxmsg.body.nondata_body);
      }
    }
  }

  acked                              = false;
  while (!acked) {
    txmsg.id                         = LTCOMM_CID_START_DATA;
    printf("sending start data\n");
    if (sendto(sd, (void *) &txmsg, sizeof(npcomm_client_request_msg_t), 0,
               (struct sockaddr *) &server_addr,
               sizeof(server_addr)) == -1) {
      printf("Network sendto failed: id=%d\n", txmsg.id);
      printf("errno: %d\n",errno);
      printf("read error: %s\n",strerror(errno));
      return -1;
    }
    rxmsg.id                                  = LTCOMM_SID_ACK;
    int client_slen                           = sizeof(struct sockaddr_in);
    if (recvfrom(sd, (void *) &rxmsg, sizeof(npcomm_server_reply_msg_t), 0,
                 (struct sockaddr *)&(client_addr),
                 (socklen_t * )&client_slen) == -1) {
      printf("Network recvfrom failed\n");
      printf("errno: %d\n",errno);
      printf("read error: %s\n",strerror(errno));
      return -1;
    }
    else {
      if (rxmsg.id == LTCOMM_SID_ACK) {
        acked = true;
        printf("received start data ack\n");
      }
    }
  }


  acked                              = false;
  while (!acked) {
    txmsg.id                         = LTCOMM_CID_REGISTER_PROGRAM_PROFILE_ID;
    printf("sending register program profile id\n");
    if (sendto(sd, (void *) &txmsg, sizeof(npcomm_client_request_msg_t), 0,
               (struct sockaddr *) &server_addr,
               sizeof(server_addr)) == -1) {
      printf("Network sendto failed: id=%d\n", txmsg.id);
      printf("errno: %d\n",errno);
      printf("read error: %s\n",strerror(errno));
      return -1;
    }
    rxmsg.id                                  = LTCOMM_SID_ACK;
    int client_slen                           = sizeof(struct sockaddr_in);
    if (recvfrom(sd, (void *) &rxmsg, sizeof(npcomm_server_reply_msg_t), 0,
                 (struct sockaddr *)&(client_addr),
                 (socklen_t * )&client_slen) == -1) {
      printf("Network recvfrom failed\n");
      printf("errno: %d\n",errno);
      printf("read error: %s\n",strerror(errno));
      return -1;
    }
    else {
      if (rxmsg.id == LTCOMM_SID_ACK) {
        acked = true;
        printf("received register program profile id ack\n");
      }
    }
  }

  /*****************************************************************************/
  /* mainloop */
  /*****************************************************************************/
  int i;
  for (i=0; i<20; i++) {
    acked                              = false;
    while (!acked) {
      txmsg.id                         = LTCOMM_CID_GET_DATA;
      printf("sending get data\n");
      if (sendto(sd, (void *) &txmsg, sizeof(npcomm_client_request_msg_t), 0,
                 (struct sockaddr *) &server_addr,
                 sizeof(server_addr)) == -1) {
        printf("Network sendto failed: id=%d\n", txmsg.id);
        printf("errno: %d\n",errno);
        printf("read error: %s\n",strerror(errno));
        return -1;
      }
      rxmsg.id                                  = LTCOMM_SID_ACK;
      int client_slen                           = sizeof(struct sockaddr_in);
      if (recvfrom(sd, (void *) &rxmsg, sizeof(npcomm_server_reply_msg_t), 0,
                   (struct sockaddr *)&(client_addr),
                   (socklen_t * )&client_slen) == -1) {
        printf("Network recvfrom failed\n");
        printf("errno: %d\n",errno);
        printf("read error: %s\n",strerror(errno));
        return -1;
      }
      else {
        if (rxmsg.id == LTCOMM_SID_DATA_REPLY) {
          acked = true;
          printf("received get data reply:\n");
          printf("  heading: %f\tpitch: %f\troll: %f\n",
                 rxmsg.body.get_data_body.heading,
                 rxmsg.body.get_data_body.pitch,
                 rxmsg.body.get_data_body.roll);
        }
      }
      sleep(1);
    }
  }

  /*****************************************************************************/
  /* shutdown */
  /*****************************************************************************/
  acked                              = false;
  while (!acked) {
    txmsg.id                         = LTCOMM_CID_DISCONNECT;
    printf("sending disconnect\n");
    if (sendto(sd, (void *) &txmsg, sizeof(npcomm_client_request_msg_t), 0,
               (struct sockaddr *) &server_addr,
               sizeof(server_addr)) == -1) {
      printf("Network sendto failed: id=%d\n", txmsg.id);
      printf("errno: %d\n",errno);
      printf("read error: %s\n",strerror(errno));
      return -1;
    }
    rxmsg.id                                  = LTCOMM_SID_ACK;
    int client_slen                           = sizeof(struct sockaddr_in);
    if (recvfrom(sd, (void *) &rxmsg, sizeof(npcomm_server_reply_msg_t), 0,
                 (struct sockaddr *)&(client_addr),
                 (socklen_t * )&client_slen) == -1) {
      printf("Network recvfrom failed\n");
      printf("errno: %d\n",errno);
      printf("read error: %s\n",strerror(errno));
      return -1;
    }
    else {
      if (rxmsg.id == LTCOMM_SID_ACK) {
        acked = true;
        printf("received disconnect ack\n");
      }
    }
  }

  return 0;
}

