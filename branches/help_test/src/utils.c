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
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>

#define IOCTL_RETRY_COUNT 5

static char *pref_file = "linuxtrack.conf";

static const char *logfile_template = "/tmp/linuxtrack%02d.log";

void ltr_int_valog_message(const char *format, va_list va)
{
  static FILE *output_stream = NULL;
  int fd;
  if(output_stream == NULL){
    char *fname = NULL;
    int cntr = 0;
    while(1){
      if(asprintf(&fname, logfile_template, cntr) < 0) return;
      output_stream = fopen(fname, "a+");
      if(output_stream != NULL){
        rewind(output_stream); //rewind to obtain lock on the whole file 
        fd = fileno(output_stream);
        if(lockf(fd, F_TLOCK, 0) == 0){
          output_stream = freopen(fname, "w+", stderr);
          free(fname);
          break;
        }
        fclose(output_stream);
      }
      free(fname);
      cntr++;
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
  size_t len1 = strlen(str1);
  size_t sum = len1 + strlen(str2) + 1; //Count trainling null too
  char *res = (char*)ltr_int_my_malloc(sum);
  strcpy(res, str1);
  strcpy(res + len1, str2);
  return res;
}

char *ltr_int_get_default_file_name(char *fname)
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
  char *pref_path = (char *)ltr_int_my_malloc(strlen(home) 
                    + strlen(fname) + strlen(pref_dir) + 3);
  sprintf(pref_path, "%s/%s/%s", home, pref_dir, fname);
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
  #define LIB_SUFFIX ".so"
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
  char *rsrc_path = (char *)ltr_int_my_malloc(strlen(section) + strlen(rsrc) + 3);
  sprintf(rsrc_path, "/%s/%s", section, rsrc);
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

#endif
