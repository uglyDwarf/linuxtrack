#include "utils.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define IOCTL_RETRY_COUNT 5


void* my_malloc(size_t size)
{
  void *ptr = malloc(size);
  if(ptr == NULL){
    log_message("Can't malloc memory! %s\n", strerror(errno));
    exit(1);
  }
  return ptr;
}

char* my_strdup(const char *s)
{
  char *ptr = strdup(s);
  if(ptr == NULL){
    log_message("Can't strdup! %s\n", strerror(errno));
    exit(1);
  }
  return ptr;
}


void log_message(const char *format, ...)
{
  static FILE *output_stream = NULL;
  
  
  if(output_stream == NULL){
    output_stream = freopen("./log.txt", "w", stderr);
    if(output_stream == NULL){
      printf("Error opening logfile!\n");
      return;
    }
  }
  va_list ap;
  va_start(ap,format);
  vfprintf(stderr, format, ap);
  fflush(stderr);
  va_end(ap);
}

int my_ioctl(int d, int request, void *argp)
{
  int cntr = 0;
  int res;
  
  do{
    res = ioctl(d, request, argp);
    if(0 == res){
      break;
    }else{
      if(errno != EIO){
        break;
      }
    }
    cntr++;
//    usleep(100);
  }while(cntr < IOCTL_RETRY_COUNT);
  return res;
}

void strlower(char *s)
{
  while (*s) {
    *s = tolower(*s);
    s++;
  }
}
