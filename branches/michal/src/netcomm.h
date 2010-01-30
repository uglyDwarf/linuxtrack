#ifndef NETCOMM__H
#define NETCOMM__H

typedef enum {INIT, SHUTDOWN, SUSPEND, WAKE, DATA} message_t;


int init_client(const char *address, const unsigned int port);
int init_server(unsigned int port);
int accept_connection(int socket);

#endif
