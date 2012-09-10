#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#ifndef LIBLINUXTRACK_SRC
  #include "utils.h"
#endif

#define MAX_LOGS 5

static char *pref_file = "linuxtrack1.conf";

static const char *logfile_template = "/tmp/linuxtrack%02d.log";
static char *logfile_name = NULL;

char* ltr_int_my_strdup(const char *s);

static void ltr_int_atexit(void)
{
  if(logfile_name != NULL){
    free(logfile_name);
    logfile_name = NULL;
  }
  fclose(stderr);
}

bool ltr_int_set_logfile(char *fname)
{
  FILE *output_stream = NULL;
  int fd;
  //The file might be opened by other process, so don't truncate
  output_stream = fopen(fname, "a+");
  if(output_stream == NULL){
    return false;
  }
  rewind(output_stream); //rewind to obtain lock on the whole file 
  fd = fileno(output_stream);
  if(lockf(fd, F_TLOCK, 0) != 0){
    fclose(output_stream);
    return false;
  }
  atexit(ltr_int_atexit);
  output_stream = freopen(fname, "w+", stderr);
  logfile_name = fname;
  return true;
}

//returns true - use immediately
//  return false - ts != exists, but usable
bool ltr_int_test_file(char *fname, time_t *ts)
{
  struct stat finfo;
  //Check file info
  if(stat(fname, &finfo) != 0){
    *ts = 0;
    if(errno == ENOENT){
      //File doesn't exist - we can use it immediately
      return true;
    }
  }
  //Other than regular files are automaticaly excluded
  if(!S_ISREG(finfo.st_mode)){
    *ts = 0;
    return false;
  }
  
  *ts = finfo.st_mtime;
  return false;
}

bool ltr_int_open_logfile()
{
  time_t timestamps[MAX_LOGS];
  char *fname = NULL;
  int cntr;
  for(cntr = 0; cntr < MAX_LOGS; ++cntr){
    if(asprintf(&fname, logfile_template, cntr) < 0){
      //most probably memory allocation failed, 
      //  and there is not much we can do about it.
      return false;
    }
    if(ltr_int_test_file(fname, &(timestamps[cntr]))){
      //We have found nonexisten file... Lets use it...
      break;
    }
    free(fname);
    fname = NULL;
  }
  
  if(fname == NULL){
    int min_index = -1;
    //find oldest one
    for(cntr = 0; cntr < MAX_LOGS; ++cntr){
      if(timestamps[cntr] == 0){
        continue;
      }
      if((min_index < 0) || (timestamps[cntr] < timestamps[min_index])){
        min_index = cntr;
      }
    }
    //construct name of the oldest one
    if(min_index >= 0){
      if(asprintf(&fname, logfile_template, min_index) < 0){
        //most probably memory allocation failed, 
        //  and there is not much we can do about it.
        return false;
      }
    }
  }
  if(fname == NULL){
    return false;
  }else{
    return ltr_int_set_logfile(fname);
  }
}

void ltr_int_valog_message(const char *format, va_list va)
{
  static bool initialized = false;
  if(!initialized){
    initialized = true;
    int cntr;
    //try four times
    for(cntr = 0; cntr < 5; ++cntr){
      if(ltr_int_open_logfile()) break;
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

void ltr_int_log_message(const char *format, ...)
{
  va_list ap;
  va_start(ap,format);
  ltr_int_valog_message(format, ap);
  va_end(ap);
}



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

#ifndef LIBLINUXTRACK_SRC

const char *ltr_int_get_logfile_name(void)
{
  return logfile_name;
}


void ltr_int_strlower(char *s)
{
  while (*s != '\0') {
    *s = tolower(*s);
    s++;
  }
}

#endif 

char *ltr_int_my_strcat(const char *str1, const char *str2)
{
  char *res = NULL;
  asprintf(&res, "%s%s", str1, str2);
  return res;
}

char *ltr_int_get_default_file_name(const char *fname)
{
  char *home = getenv("HOME");
  char *pref_dir = ".linuxtrack";
  if(home == NULL){
    ltr_int_log_message("Please set HOME variable!\n");
    return NULL;
  }
  if(fname == NULL){
    fname = pref_file;
  }
  char *pref_path = NULL;
  asprintf(&pref_path, "%s/%s/%s", home, pref_dir, fname);
  return pref_path;
}

char *ltr_int_get_app_path(const char *suffix)
{
  char *fname = ltr_int_get_default_file_name(NULL);
  if(fname == NULL){
    return NULL;
  }
  FILE *f = fopen(fname, "r");
  if(f == NULL){
    ltr_int_log_message("Can't open file '%s'!\n", fname);
    free(fname);
    return NULL;
  }
  
  free(fname);
  fname = NULL;
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

#ifndef LIBLINUXTRACK_SRC


#ifndef DARWIN
  #define DATA_PATH "/../share/linuxtrack/"
  #define LIB_SUFFIX ".so.0"
#else
  #undef LIB_PATH
  #define DATA_PATH "/../Resources/linuxtrack/"
  #define LIB_PATH "/../Frameworks/"
  #define LIB_SUFFIX ".0.dylib"
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

char *ltr_int_get_data_path_prefix(const char *data, const char *prefix)
{
  char *app_path = ltr_int_my_strcat(prefix, DATA_PATH);
  char *data_path = ltr_int_my_strcat(app_path, data);
  free(app_path);
  return data_path;
}

char *ltr_int_get_lib_path(const char *libname)
{
  #ifdef DARWIN
    char *app_path = ltr_int_get_app_path(LIB_PATH);
    if(app_path == NULL){
      return NULL;
    }
  #else
    char *app_path = ltr_int_my_strdup(LIB_PATH);
  #endif
    char *lib_path1 = ltr_int_my_strcat(app_path, libname);
    char *lib_path = ltr_int_my_strcat(lib_path1, LIB_SUFFIX);
    free(app_path);
    free(lib_path1);
  return lib_path;
}

char *ltr_int_get_resource_path(const char *section, const char *rsrc)
{
  char *rsrc_path = NULL;
  asprintf(&rsrc_path, "/%s/%s", section, rsrc);
  char *path = ltr_int_get_default_file_name(rsrc_path);
  FILE *f = fopen(path, "rb");
  if(f != NULL){
    fclose(f);
    free(rsrc_path);
    return path;
  }
  path = ltr_int_get_data_path(rsrc);
  fopen(path, "rb");
  if(f != NULL){
    fclose(f);
    return path;
  }
  return NULL;
}


dbg_flag_type ltr_int_get_dbg_flag(const int flag)
{
  char *dbg_flags = getenv("LINUXTRACK_DBG");
  if(dbg_flags == NULL) return DBG_OFF;
  if(strchr(dbg_flags, flag) != NULL){
    return DBG_ON;
  }else{
    return DBG_OFF;
  }
}

void ltr_int_usleep(unsigned int usec)
{
  struct timespec req, rem;
  
  req.tv_sec = usec / 1000000;
  req.tv_nsec = (usec % 1000000) * 1000;
  while(1){
    if(nanosleep(&req, &rem) != -1){
      break;
    }else{
      if(errno == EINTR){
        req = rem;
        continue;
      }else{
        //Here the other handler would come;
        //But how to handle this?
        perror("nanosleep: ");
        break;
      }
    }
  }
}
#endif



