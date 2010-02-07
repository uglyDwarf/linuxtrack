#ifndef NETCOMM__H
#define NETCOMM__H

#include "cal.h"

typedef enum {RUN, SHUTDOWN, SUSPEND, WAKE, DATA} message_t;


int init_client(const char *address, const unsigned int port);
int init_server(unsigned int port);
int accept_connection(int socket);
size_t encode_bloblist(struct bloblist_type *blobs, unsigned char *buffer);
void decode_bloblist(struct bloblist_type *blobs, unsigned char *buffer);
#endif
