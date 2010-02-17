
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "cal.h"

int init_client(const char *name, unsigned int port,
                unsigned int restart_timeout)
{
  int sfd;
  struct addrinfo *ai;
  struct addrinfo hint;

  //get address info...  
  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_INET;
  hint.ai_socktype = SOCK_STREAM;
  int status = getaddrinfo(name, NULL, &hint, &ai);
  if(status != 0){
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return -1;
  }
  
  //try to open the socket (examine all possible results)
  struct addrinfo *tmp_ai;
  struct sockaddr_in *sinp;
  struct sockaddr_in inaddr;
  for(tmp_ai = ai; tmp_ai != NULL; tmp_ai = tmp_ai->ai_next){
    char addr[1024];
    sinp = (struct sockaddr_in*)tmp_ai->ai_addr;
    inet_ntop(ai->ai_family, &sinp->sin_addr, addr, sizeof(addr));
    fprintf(stderr, "Have result! %s\n", addr);

    sinp->sin_port = htons(port);
    inaddr = *sinp;
    sfd = socket(((struct sockaddr*)sinp)->sa_family, SOCK_STREAM, 0);
    printf("Socket %d\n", sfd);
    if(sfd < 0){
      perror("socket");
      continue;
    }
    break;
  }
  
  if(tmp_ai == NULL){
    fprintf(stderr, "Didn't work!\n");
    return -1;
  }
  
  freeaddrinfo(ai);
  while(1){
  status = connect(sfd, (struct sockaddr*)&inaddr, sizeof(inaddr));
  printf("Connecting...\n");
  if(status != 0){
    if(errno == EINTR){
      continue;
    }else{
      perror("connect");
      if(restart_timeout != 0){
        sleep(restart_timeout);
      }
      return -1;
    }
  }
  }
  return sfd;
}


/*
initializes server

type - e.g. SOCK_STREAM, SOCK_DGRAM... see 'man socket' (optionaly SOCK_NONBLOCK)
addr
*/
int prepare_server_comm(int type, const struct sockaddr *addr, socklen_t addr_len, 
               int qlen)
{
  int fd;
  int reuse = 1;
  
  fd = socket(addr->sa_family, type, 0);
  if(fd < 0){
    perror("socket:");
    return -1;
  }
  if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0){
    perror("setsockopt:");
    goto error_label;
  }
  
  if(bind(fd, addr, addr_len) != 0){
    perror("bind:");
    goto error_label;
  }
  
  if((type == SOCK_STREAM) || (type == SOCK_SEQPACKET)){
    if(listen(fd, qlen) < 0){
      perror("listen:");
      goto error_label;
    }
  }
  return fd;
  
  //Handle error...
 error_label:
  close(fd);
  return -1;
}

int init_server(unsigned int port)
{
  int sfd;
  
  char name[1024];
  struct addrinfo *ai;
  struct addrinfo hint;
  
  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_INET;
  hint.ai_socktype = SOCK_STREAM;
  
  if(gethostname(name, sizeof(name)) != 0){
    perror("gethostname");
  }else{
    strcpy(name, "localhost");
  }
  
  int status = getaddrinfo(name, NULL, &hint, &ai);
  if(status != 0){
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return -1;
  }
  
  struct addrinfo *tmp_ai;
  for(tmp_ai = ai; tmp_ai != NULL; tmp_ai = tmp_ai->ai_next){
      struct sockaddr_in *sinp = (struct sockaddr_in*)tmp_ai->ai_addr;
      
      sinp->sin_addr.s_addr = htonl(INADDR_ANY);
      sinp->sin_port = htons(port);
      sfd = prepare_server_comm(SOCK_STREAM, (struct sockaddr*)sinp, 
                                tmp_ai->ai_addrlen, 0);
      if(sfd != -1){
        break;
      }
  }
  
  if(tmp_ai == NULL){
    fprintf(stderr, "Didn't work!\n");
    return -1;
  }
  
  freeaddrinfo(ai);
  return sfd;
}

int accept_connection(int socket)
{
  int cfd;
  struct sockaddr_in client;
  socklen_t client_len = sizeof(struct sockaddr);
  printf("Going to wait!\n");
  while(1){
    cfd = accept(socket, (struct sockaddr*)&client, &client_len);
    if(cfd == -1){
      if(errno == EINTR){
        continue;
      }else{
        perror("accept");
        return -1;
      }
    }else{
      break;
    }
    
  }
  return cfd;
}

void encode_float(float *f, unsigned char *buffer)
{
  unsigned int *uf = (unsigned int*)f;
  buffer[0] = *uf & 0xFF;
  buffer[1] = (*uf >> 8) & 0xFF;
  buffer[2] = (*uf >> 16) & 0xFF;
  buffer[3] = (*uf >> 24) & 0xFF;
//  printf("%g -> %X\n", *f, *uf);
}

void decode_float(unsigned char *buffer, float *f)
{
  unsigned int uf = 0;
  uf = buffer[3];
  uf <<= 8;
  uf += (buffer[2] & 0xFF);
  uf <<= 8;
  uf += (buffer[1] & 0xFF);
  uf <<= 8;
  uf += (buffer[0] & 0xFF);
  *f = *(float*)(&uf);
//  printf("%g -> %X\n", *f, uf);
}

size_t encode_bloblist(struct bloblist_type *blobs, unsigned char *buffer)
{
  buffer[0] = (blobs->num_blobs) * sizeof(float);
  ++buffer;
  size_t cntr = 1;
  int i;
  for(i = 0; i < blobs->num_blobs; ++i){
    encode_float(&((blobs->blobs)[i].x), buffer);
    //printf("%g => %02X %02X %02X %02X\n", ((blobs->blobs)[i].x), buffer[0], buffer[1], buffer[2], buffer[3]);
    buffer += 4;
    encode_float(&((blobs->blobs)[i].y), buffer);
    //printf("%g => %02X %02X %02X %02X\n", ((blobs->blobs)[i].y), buffer[0], buffer[1], buffer[2], buffer[3]);
    buffer += 4;
    cntr += 8;
  }
  return cntr;
}

void decode_bloblist(struct bloblist_type *blobs, unsigned char *buffer)
{
  int data_size = buffer[0] / sizeof(float);
  blobs->num_blobs = (data_size<=3) ? data_size : 3;
//  printf("Got %d blobs!\n", blobs->num_blobs);
  ++buffer;
  int i;
  for(i = 0; i < blobs->num_blobs; ++i){
    decode_float(buffer, &((blobs->blobs)[i].x));
    //printf("%g => %02X %02X %02X %02X\n", ((blobs->blobs)[i].x), buffer[0], buffer[1], buffer[2], buffer[3]); 
    buffer += 4;
    decode_float(buffer, &((blobs->blobs)[i].y));
    //printf("%g => %02X %02X %02X %02X\n", ((blobs->blobs)[i].y), buffer[0], buffer[1], buffer[2], buffer[3]);
    buffer += 4;
  }
}
