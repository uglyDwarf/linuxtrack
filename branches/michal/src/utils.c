#include "utils.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

#define IOCTL_RETRY_COUNT 5


void* ltr_int_my_malloc(size_t size)
{
  void *ptr = malloc(size);
  if(ptr == NULL){
    ltr_int_log_message("Can't malloc memory! %s\n", strerror(errno));
    assert(0);
    exit(1);
  }
  return ptr;
}

char* ltr_int_my_strdup(const char *s)
{
  char *ptr = strdup(s);
  if(ptr == NULL){
    ltr_int_log_message("Can't strdup! %s\n", strerror(errno));
    exit(1);
  }
  return ptr;
}

void ltr_int_log_message(const char *format, ...)
{
  va_list ap;
  va_start(ap,format);
  ltr_int_valog_message(format, ap);
  va_end(ap);
}

void ltr_int_valog_message(const char *format, va_list va)
{
  static FILE *output_stream = NULL;
  if(output_stream == NULL){
    output_stream = freopen("/tmp/linuxtrack.log", "w", stderr);
    if(output_stream == NULL){
      printf("Error opening logfile!\n");
      return;
    }
  }
  time_t now = time(NULL);
  struct tm  *ts = localtime(&now);
  char       buf[80];
  strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
  
  fprintf(stderr, "[%s] ", buf);
  vfprintf(stderr, format, va);
  fflush(stderr);
}


int ltr_int_my_ioctl(int d, int request, void *argp)
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

void ltr_int_strlower(char *s)
{
  while (*s != '\0') {
    *s = tolower(*s);
    s++;
  }
}

char *ltr_int_my_strcat(const char *str1, const char *str2)
{
  size_t len1 = strlen(str1);
  size_t sum = len1 + strlen(str2) + 1; //Count trainling null too
  char *res = (char*)ltr_int_my_malloc(sum);
  strcpy(res, str1);
  strcpy(res + len1, str2);
  return res;
}

