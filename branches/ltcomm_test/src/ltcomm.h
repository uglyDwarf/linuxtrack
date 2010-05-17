#ifndef LTCOMM__H
#define LTCOMM__H 1

#include <stdint.h>

#define LTCOMM_DEFAULT_CLIENT_PORT 12012
#define LTCOMM_DEFAULT_SERVER_PORT 12013
#define LTCOMM_DEFAULT_HOST        "127.0.0.1"

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

typedef struct {
  uint8_t  id;
  uint16_t body; // optional, only used for register_prog_id and get_param
}  npcomm_client_request_msg_t ;

/*** SERVER REPLY MESSAGES ***/
/*** SERVER REPLY MESSAGES ***/
#define  LTCOMM_SID_DATA_REPLY    201 // SUCCESS with data for get_data
#define  LTCOMM_SID_VERSION_REPLY 202 // SUCCESS with version data for query_version
#define  LTCOMM_SID_PARAM_REPLY   203 // SUCCESS with param for get_param
#define  LTCOMM_SID_ACK           204 // SUCCESS for others
#define  LTCOMM_SID_NACK          205 // FAILED

#define  LTCOMM_ERR_INVALID_MSG 001

typedef struct {
  float pitch;
  float roll;
  float heading;
  float tx;
  float ty;
  float tz;
} npcomm_get_data_coords_t;

typedef union {
  npcomm_get_data_coords_t get_data_body;
  uint32_t                 nondata_body;
} npcomm_server_reply_body_t;

typedef struct {
  uint8_t                    id;
  npcomm_server_reply_body_t body;
}  npcomm_server_reply_msg_t ;

#endif
