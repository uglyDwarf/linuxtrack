#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "ltlib_int.h"
#include "utils.h"
#include "dyn_load.h"

static int (*fun_lt_int_init)(char *cust_section) = NULL;
static int (*fun_lt_int_shutdown)(void) = NULL;
static int (*fun_lt_int_suspend)(void) = NULL;
static int (*fun_lt_int_wakeup)(void) = NULL;
static void (*fun_lt_int_recenter)(void) = NULL;
static int (*fun_lt_int_get_camera_update)(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz) = NULL;
static lt_state_type (*fun_lt_int_get_tracking_state)(void) = NULL;
static void (*fun_lt_int_log_message)(const char *format, ...) = NULL;


static lib_fun_def_t functions[] = {
{"lt_int_init", (void*) &fun_lt_int_init},
{"lt_int_shutdown", (void*) &fun_lt_int_shutdown},
{"lt_int_suspend", (void*) &fun_lt_int_suspend},
{"lt_int_wakeup", (void*) &fun_lt_int_wakeup},
{"lt_int_recenter", (void*) &fun_lt_int_recenter},
{"lt_int_get_camera_update", (void*) &fun_lt_int_get_camera_update},
{"lt_int_get_tracking_state", (void*) &fun_lt_int_get_tracking_state},
{"lt_int_log_message", (void*) &fun_lt_int_log_message},
{NULL, NULL}
};

static void *libhandle = NULL;
static char libname[] = "liblinuxtrack.so";

int lt_init(char *cust_section)
{
  if(fun_lt_int_init == NULL){
    if((libhandle = lt_load_library(libname, functions)) == NULL){
      log_message("Problem loading library %s!\n", libname);
      return -1;
    }
  }
  return fun_lt_int_init(cust_section);
}

int lt_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz)
{
  if(fun_lt_int_get_camera_update == NULL){
    if((libhandle = lt_load_library(libname, functions)) != NULL){
      return -1;
    }
  }
  return fun_lt_int_get_camera_update(heading, pitch, roll, tx, ty, tz);
}

int lt_suspend(void)
{
  if(fun_lt_int_suspend == NULL){
    if((libhandle = lt_load_library(libname, functions)) != NULL){
      return -1;
    }
  }
  return fun_lt_int_suspend();
}

int lt_wakeup(void)
{
  if(fun_lt_int_wakeup == NULL){
    if((libhandle = lt_load_library(libname, functions)) != NULL){
      return -1;
    }
  }
  return fun_lt_int_wakeup();
}

int lt_shutdown(void)
{
  if(fun_lt_int_shutdown == NULL){
    if((libhandle = lt_load_library(libname, functions)) != NULL){
      return -1;
    }
  }
  int res = fun_lt_int_shutdown();
  lt_unload_library(libhandle, functions);
  return res;
}

void lt_recenter(void)
{
  if(fun_lt_int_recenter == NULL){
    if((libhandle = lt_load_library(libname, functions)) != NULL){
      return;
    }
  }
  return fun_lt_int_recenter();
}

lt_state_type lt_get_tracking_state(void)
{
  if(fun_lt_int_get_tracking_state == NULL){
    if((libhandle = lt_load_library(libname, functions)) != NULL){
      return STOPPED;
    }
  }
  return fun_lt_int_get_tracking_state();
}

void lt_log_message(const char *format, ...)
{
  if(fun_lt_int_log_message == NULL){
    if((libhandle = lt_load_library(libname, functions)) != NULL){
      return;
    }
  }
  va_list ap;
  va_start(ap,format);
  fun_lt_int_log_message(format, ap);
  va_end(ap);
}
