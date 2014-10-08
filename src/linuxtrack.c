/*
The MIT License

Copyright (c) 2009 Tulthix, uglyDwarf

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/





#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include "linuxtrack.h"

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#define BLOB_ELEMENTS 3

typedef int (*ltr_gp_t)(void);
typedef int (*ltr_init_t)(const char *cust_section);
typedef int (*ltr_get_pose_t)(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz,
                         uint32_t *counter);
typedef int (*ltr_get_pose_full_t)(linuxtrack_pose_t *pose, float blobs[], int num_blobs, int *blobs_read);
typedef linuxtrack_state_type (*ltr_get_tracking_state_t)(void);
typedef const char *(*ltr_explain_t)(linuxtrack_state_type);


static ltr_init_t ltr_init_fun = NULL;
static ltr_gp_t ltr_shutdown_fun = NULL;
static ltr_gp_t ltr_suspend_fun = NULL;
static ltr_gp_t ltr_wakeup_fun = NULL;
static ltr_gp_t ltr_recenter_fun = NULL;
static ltr_get_pose_t ltr_get_pose_fun = NULL;
static ltr_get_pose_full_t ltr_get_pose_full_fun = NULL;
static ltr_get_tracking_state_t ltr_get_tracking_state_fun = NULL;
static ltr_explain_t ltr_explain_fun = NULL;

static void *lib_handle = NULL;

struct func_defs_t{
  char *name;
  void *ref;
  int mandatory;
};

static struct func_defs_t functions[] =
{
  {(char*)"ltr_init", (void*)&ltr_init_fun, 1},
  {(char*)"ltr_shutdown", (void*)&ltr_shutdown_fun, 1},
  {(char*)"ltr_suspend", (void *)&ltr_suspend_fun, 1},
  {(char*)"ltr_wakeup", (void *)&ltr_wakeup_fun, 1},
  {(char*)"ltr_recenter", (void *)&ltr_recenter_fun, 1},
  {(char*)"ltr_get_pose", (void *)&ltr_get_pose_fun, 1},
  {(char*)"ltr_get_pose_full", (void *)&ltr_get_pose_full_fun, 1},
  {(char*)"ltr_get_tracking_state", (void *)&ltr_get_tracking_state_fun, 1},
  {(char*)"ltr_explain", (void *)&ltr_explain_fun, 0},
  {(char*)NULL, NULL, 0}
};

static const char *lib_locations[] = {
"/Frameworks/liblinuxtrack.0.dylib",
"/lib/linuxtrack/liblinuxtrack.so.0",
"/lib32/linuxtrack/liblinuxtrack32.so.0",
"/lib/i386-linux-gnu/linuxtrack/liblinuxtrack.so.0",
"/lib/i386-linux-gnu/linuxtrack/liblinuxtrack32.so.0",
"/lib/x86_64-linux-gnu/linuxtrack/liblinuxtrack.so.0",
/* old paths */
"/lib/liblinuxtrack.so.0",
"/lib32/liblinuxtrack32.so.0",
"/lib/i386-linux-gnu/liblinuxtrack.so.0",
"/lib/x86_64-linux-gnu/liblinuxtrack.so.0",
NULL
};

static FILE *log_f = NULL;
static char logfname[] = "/tmp/linuxtrack.logXXXXXX";

static void linuxtrack_log(const char *format, ...)
{
  if(log_f == NULL){
    FILE *tmpf;
    int tmpfd = mkstemp(logfname);
    if(tmpfd != -1){
      tmpf = fdopen(tmpfd, "a");
      if(tmpf != NULL){
        log_f = tmpf;
      }
    }
  }
  va_list ap;
  va_start(ap,format);
  vfprintf(log_f, format, ap);
  fflush(log_f);
  va_end(ap);
}

int linuxtrack_shutdown(void)
{
  int res;
  if(ltr_shutdown_fun == NULL){
    return err_NOT_INITIALIZED;
  }
  res = ltr_shutdown_fun();

  if(lib_handle != NULL){
    void *handle = lib_handle;
    lib_handle = NULL;
    ltr_init_fun = NULL;
    ltr_shutdown_fun = NULL;
    ltr_suspend_fun = NULL;
    ltr_wakeup_fun = NULL;
    ltr_recenter_fun = NULL;
    ltr_get_pose_fun = NULL;
    ltr_get_pose_full_fun = NULL;
    ltr_get_tracking_state_fun = NULL;
    ltr_explain_fun = NULL;
    dlclose(handle);
  }
  return res;
}

int linuxtrack_suspend(void)
{
  if(ltr_suspend_fun == NULL){
    return err_NOT_INITIALIZED;
  }
  return ltr_suspend_fun();
}

int linuxtrack_wakeup(void)
{
  if(ltr_wakeup_fun == NULL){
    return err_NOT_INITIALIZED;
  }
  return ltr_wakeup_fun();
}

int linuxtrack_recenter(void)
{
  if(ltr_recenter_fun == NULL){
    return err_NOT_INITIALIZED;
  }
  return ltr_recenter_fun();
}


//RetVal 0 means no new data
int linuxtrack_get_pose(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz,
                         uint32_t *counter)
{
  if(ltr_get_pose_fun == NULL){
    *heading = *pitch = *roll = *tx = *ty = *tz = 0.0f;
    *counter = 0;
    return 0;
  }
  return ltr_get_pose_fun(heading, pitch, roll, tx, ty, tz, counter);
}

//RetVal 0 means no new data
int linuxtrack_get_pose_full(linuxtrack_pose_t *pose, float blobs[], int num_blobs, int *blobs_read)
{
  if(ltr_get_pose_full_fun == NULL){
    memset(pose, 0, sizeof(linuxtrack_pose_t));
    int i;
    for(i = 0; i < num_blobs * BLOB_ELEMENTS; ++i){
      blobs[i] = 0.0f;
    }
    return 0;
  }
  return ltr_get_pose_full_fun(pose, blobs, num_blobs, blobs_read);
}


linuxtrack_state_type linuxtrack_get_tracking_state(void)
{
  if(ltr_get_tracking_state_fun == NULL){
    return err_NOT_INITIALIZED;
  }
  return ltr_get_tracking_state_fun();
}


static int linuxtrack_load_functions(void *handle)
{
  int i = 0;
  void *symbol;
  while((functions[i]).name != NULL){
    dlerror();
    if((symbol = dlsym(handle, (functions[i]).name)) == NULL){
      linuxtrack_log("Couldn't load symbol '%s': %s\n", (functions[i]).name, dlerror());
      if((functions[i]).mandatory){
        return err_SYMBOL_LOOKUP;
      }
    }
    *((void **)(functions[i]).ref) = symbol;
    ++i;
  }
  return LINUXTRACK_OK;
}


static char *construct_name(const char *path, const char *sep, const char *name)
{
  size_t len = strlen(path) + strlen(sep) + strlen(name) + 1;
  char *res = (char *)malloc(len);
  snprintf(res, len, "%s%s%s", path, sep, name);
  return res;
}


static void* linuxtrack_try_library(const char *path)
{
  void *handle = NULL;
  linuxtrack_log("Trying to load '%s'... ", path);
  if(access(path, F_OK) != 0){
    linuxtrack_log("Not found.\n");
    return NULL;
  }
  dlerror();
  handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if(handle != NULL){
    linuxtrack_log("Loaded OK.\n");
    return handle;
  }
  linuxtrack_log("Couldn't load library - %s!\n", dlerror());
  return NULL;
}

char *linuxtrack_get_prefix()
{
  char *prefix = NULL;
  char *home = getenv("HOME");
  char *cfg = (char*)"/.config/linuxtrack/linuxtrack1.conf";
  char *fname;
  FILE *f;
  char *line;
  char *val, *key;

  if(home == NULL){
    linuxtrack_log("Please set HOME variable!\n");
    return NULL;
  }
  fname = construct_name(home, cfg, "");
  if((f = fopen(fname, "r")) == NULL){
    free(fname);
    return NULL;
  }
  free(fname);
  line = (char *)malloc(4096);
  val = (char *)malloc(4096);
  key = (char *)malloc(4096);
  while(!feof(f)){
    if(fgets(line, 4095, f) != NULL){
      if((sscanf(line, "%s = \"%[^\"\n]", key, val) == 2) &&
        strcasecmp(key, "prefix") == 0){
	prefix = strdup(val);
	break;
      }
    }
  }
  fclose(f);
  free(line);
  free(val);
  free(key);
  return prefix;
}



static void* linuxtrack_find_library(linuxtrack_state_type *problem)
{
  /*
  //search order:
  //  1. LINUXTRACK_LIBS
  //       development, backward compatibility and weird locations handling
  //  2. prefix from config file
  //  3. plain libname
  //       worth in Linux only, since on Mac we never install to system libraries
  */
  void *handle = NULL;
  char *name = NULL;
  char *prefix;
  /*Look for LINUXTRACK_LIBS*/
  char *lp = getenv("LINUXTRACK_LIBS");
  if(lp != NULL){
    char *path = strdup(lp);
    char *part = path;
    while(1){
      part = strtok(part, ":");
      if((part == NULL) || ((handle = linuxtrack_try_library(part)) != NULL)){
        break;
      }
      part = NULL;
    }
    free(path);
    if(handle != NULL){
      return handle;
    }
  }

  prefix = linuxtrack_get_prefix();
  if(prefix == NULL){
    *problem = err_NO_CONFIG;
    return NULL;
  }
  int i = 0;
  while(lib_locations[i] != NULL){
    name = construct_name(prefix, "/../", lib_locations[i++]);
    if((handle = linuxtrack_try_library(name)) != NULL){
      free(name);
      free(prefix);
      return handle;
    }
    free(name);
  }
  free(prefix);
  *problem = err_NOT_FOUND;
  return NULL;
}



static linuxtrack_state_type linuxtrack_load_library()
{
  linuxtrack_state_type problem;
  lib_handle = linuxtrack_find_library(&problem);
  if(lib_handle == NULL){
    linuxtrack_log("Couldn't load liblinuxtrack, headtracking will not be available!\n");
    return problem;
  }
  dlerror(); /*clear any existing error...*/
  if(linuxtrack_load_functions(lib_handle) != LINUXTRACK_OK){
    linuxtrack_log("Couldn't load liblinuxtrack functions, headtracking will not be available!\n");
    return err_SYMBOL_LOOKUP;
  }
  return LINUXTRACK_OK;
}

int linuxtrack_init(const char *cust_section)
{
  linuxtrack_state_type res = linuxtrack_load_library();
  if(res < LINUXTRACK_OK){
    return res;
  }
  return ltr_init_fun(cust_section);
}

const char *linuxtrack_explain(linuxtrack_state_type status)
{
  if(ltr_explain_fun != NULL){
    return ltr_explain_fun(status);
  }
  char *res = NULL;
  switch(status){
    case INITIALIZING:
      res = "Linuxtrack is initializing.";
      break;
    case RUNNING:
      res = "Linuxtrack is running.";
      break;
    case PAUSED:
      res = "Linuxtrack is paused.";
      break;
    case STOPPED:
      res = "Linuxtrack is stopped.";
      break;
    case err_NOT_INITIALIZED:
      res = "Linuxtrack function was called without proper initialization.";
      break;
    case err_SYMBOL_LOOKUP:
      res = "Internal error (symbol lookup). Please file an issue at Linuxtrack project page.";
      break;
    case err_NO_CONFIG:
      res = "Linuxtrack config not found. If you have installed Linuxtrack,\n"
            "run ltr_gui and set it up first.";
      break;
    case err_NOT_FOUND:
      res = "Linuxtrack was removed or relocated. If you relocated it,\n"
            "run ltr_gui from the new location, save preferences and try again.";
      break;
    case err_PROCESSING_FRAME:
      res = "Internal error (frame processing). Please file an issue at Linuxtrack project page.";
      break;
    default:
      printf("UNKNOWN status code. Please file an issue at Linuxtrack project page.\n");
     break;
  }
  return res;
}
