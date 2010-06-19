#ifdef HAVE_CONFIG_H
  #include "../config.h"
#endif

#include "utils.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

#define IOCTL_RETRY_COUNT 5

static char *pref_file = ".linuxtrack";

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

char *ltr_int_get_default_file_name()
{
  char *home = getenv("HOME");
  if(home == NULL){
    ltr_int_log_message("Please set HOME variable!\n");
    return NULL;
  }
  char *pref_path = (char *)ltr_int_my_malloc(strlen(home) 
                    + strlen(pref_file) + 2);
  sprintf(pref_path, "%s/%s", home, pref_file);
  return pref_path;
}

char *ltr_int_get_app_path(const char *suffix)
{
  char *fname = ltr_int_get_default_file_name();
  if(fname == NULL){
    return NULL;
  }
  FILE *f = fopen(fname, "r");
  if(f == NULL){
    ltr_int_log_message("Can't open file '%s'!\n", fname);
    return NULL;
  }
  
  char key[2048];
  char val[2048];
  bool found = false;
  while(!feof(f)){
    if(fscanf(f, "%2040s", key) == 1){
      if(strcasecmp(key, "PREFIX") == 0){
	if(fgets(key, 2040, f) != NULL){
	  if(sscanf(key, " = \"%[^\"\n]", val) > 0){
	    found = true;
	    break;
	  }
	}
      }
    }
  }
  fclose(f);
  if(found){
    return ltr_int_my_strcat(val, suffix);
  }
  ltr_int_log_message("Couldn't find prefix!\n");
  return NULL;
}

#ifndef DARWIN
  #define DATA_PATH "/../share/linuxtrack/"
  #define LIB_PATH "/../lib/"
#else
  #define DATA_PATH "/../Resources/linuxtrack/"
  #define LIB_PATH "/../Frameworks/linuxtrack.framework/Versions/1/"
#endif

char *ltr_int_get_data_path(const char *data)
{
  char *app_path = ltr_int_get_app_path(DATA_PATH);
  if(app_path == NULL){
    return NULL;
  }
  char *data_path = ltr_int_my_strcat(app_path, data);
  free(app_path);
  return data_path;
}

char *ltr_int_get_lib_path(const char *libname)
{
  char *app_path = ltr_int_get_app_path(LIB_PATH);
  if(app_path == NULL){
    return NULL;
  }
  char *lib_path = ltr_int_my_strcat(app_path, libname);
  free(app_path);
  return lib_path;
}

