#ifndef LTCOMM__H
#define LTCOMM__H 1

#include <stdint.h>
/* for network */
#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

/*** COMMON ***/
/*** COMMON ***/
#define LTCOMM_DEFAULT_CLIENT_PORT 12012
#define LTCOMM_DEFAULT_SERVER_PORT 12013
#define LTCOMM_DEFAULT_HOST        "127.0.0.1"

typedef struct {
  int                 sd;
#ifndef WIN32
  struct  sockaddr_in server_addr;
  struct  sockaddr_in client_addr;
#else
  SOCKADDR_IN server_addr;
  SOCKADDR_IN client_addr;
#endif
} ltcomm_net_ctrl_blk_t;

/*** CLIENT INITIATE MESSAGES ***/
/*** CLIENT INITIATE MESSAGES ***/
#define LTCOMM_CID_CONNECT                     101
#define LTCOMM_CID_QUERY_VERSION               102
#define LTCOMM_CID_GET_PARAM                   103
#define LTCOMM_CID_START_DATA                  104
#define LTCOMM_CID_REGISTER_PROGRAM_PROFILE_ID 105
#define LTCOMM_CID_STOP_DATA                   106
#define LTCOMM_CID_RECENTER                    107
#define LTCOMM_CID_GET_DATA                    108
#define LTCOMM_CID_DISCONNECT                  109
#define LTCOMM_CID_TERMINATE                   110

#define LTCOMM_CID2STR(x,s) do { x == LTCOMM_CID_CONNECT                     ? strcpy(s,"LTCOMM_CID_CONNECT") : \
      x                            == LTCOMM_CID_QUERY_VERSION               ? strcpy(s,"LTCOMM_CID_QUERY_VERSION") : \
      x                            == LTCOMM_CID_GET_PARAM                   ? strcpy(s,"LTCOMM_CID_GET_PARAM") : \
      x                            == LTCOMM_CID_START_DATA                  ? strcpy(s,"LTCOMM_CID_START_DATA") : \
      x                            == LTCOMM_CID_REGISTER_PROGRAM_PROFILE_ID ? strcpy(s,"LTCOMM_CID_REGISTER_PROGRAM_PROFILE_ID") : \
      x                            == LTCOMM_CID_STOP_DATA                   ? strcpy(s,"LTCOMM_CID_STOP_DATA") : \
      x                            == LTCOMM_CID_RECENTER                    ? strcpy(s,"LTCOMM_CID_RECENTER") : \
      x                            == LTCOMM_CID_GET_DATA                    ? strcpy(s,"LTCOMM_CID_GET_DATA") : \
      x                            == LTCOMM_CID_DISCONNECT                  ? strcpy(s,"LTCOMM_CID_DISCONNECT") : \
      x                            == LTCOMM_CID_TERMINATE                   ? strcpy(s,"LTCOMM_CID_TERMINATE") : \
      strcpy(s,"UNKNOWN");                                              \
  } while (0);

typedef struct {
  uint8_t  id;
  uint16_t body; // optional, only used for register_prog_id and get_param
}  ltcomm_client_request_msg_t ;

/*** SERVER REPLY MESSAGES ***/
/*** SERVER REPLY MESSAGES ***/
#define  LTCOMM_SID_DATA_REPLY    201 // SUCCESS with data for get_data
#define  LTCOMM_SID_VERSION_REPLY 202 // SUCCESS with version data for query_version
#define  LTCOMM_SID_PARAM_REPLY   203 // SUCCESS with param for get_param
#define  LTCOMM_SID_ACK           204 // SUCCESS for others
#define  LTCOMM_SID_NACK          205 // FAILED

#define LTCOMM_SID2STR(x,s) do { x == LTCOMM_SID_DATA_REPLY    ? strcpy(s,"LTCOMM_SID_DATA_REPLY") : \
      x                            == LTCOMM_SID_VERSION_REPLY ? strcpy(s,"LTCOMM_SID_VERSION_REPLY") : \
      x                            == LTCOMM_SID_PARAM_REPLY   ? strcpy(s,"LTCOMM_SID_PARAM_REPLY") : \
      x                            == LTCOMM_SID_ACK           ? strcpy(s,"LTCOMM_SID_ACK") : \
      x                            == LTCOMM_SID_NACK          ? strcpy(s,"LTCOMM_SID_NACK") : \
      strcpy(s,"UNKNOWN");                                              \
  } while (0);


#define  LTCOMM_ERR_INVALID_MSG 001

typedef struct {
  float pitch;
  float roll;
  float heading;
  float tx;
  float ty;
  float tz;
} ltcomm_get_data_coords_t;

typedef union {
  ltcomm_get_data_coords_t data_body;
  uint32_t                 version_body;
  uint32_t                 param_body;
  uint32_t                 error_body;
} ltcomm_server_reply_body_t;

typedef struct {
  uint8_t                    id;
  ltcomm_server_reply_body_t body;
}  ltcomm_server_reply_msg_t ;

#endif
