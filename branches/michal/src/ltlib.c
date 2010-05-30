#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "ltlib_int.h"
#include "utils.h"
#include "dyn_load.h"

static int (*fun_ltr_int_init)(char *cust_section) = NULL;
static int (*fun_ltr_int_shutdown)(void) = NULL;
static int (*fun_ltr_int_suspend)(void) = NULL;
static int (*fun_ltr_int_wakeup)(void) = NULL;
static void (*fun_ltr_int_recenter)(void) = NULL;
static int (*fun_ltr_int_get_camera_update)(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz) = NULL;
static ltr_state_type (*fun_ltr_int_get_tracking_state)(void) = NULL;


static lib_fun_def_t functions[] = {
{"ltr_int_init", (void*) &fun_ltr_int_init},
{"ltr_int_shutdown", (void*) &fun_ltr_int_shutdown},
{"ltr_int_suspend", (void*) &fun_ltr_int_suspend},
{"ltr_int_wakeup", (void*) &fun_ltr_int_wakeup},
{"ltr_int_recenter", (void*) &fun_ltr_int_recenter},
{"ltr_int_get_camera_update", (void*) &fun_ltr_int_get_camera_update},
{"ltr_int_get_tracking_state", (void*) &fun_ltr_int_get_tracking_state},
{NULL, NULL}
};

static void *libhandle = NULL;
static char libname[] = "liblinuxtrack.so";

int ltr_init(char *cust_section)
{
  if(fun_ltr_int_init == NULL){
    if((libhandle = ltr_int_load_library(libname, functions)) == NULL){
      ltr_int_log_message("Problem loading library %s!\n", libname);
      return -1;
    }
  }
  return fun_ltr_int_init(cust_section);
}

int ltr_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz)
{
  if(fun_ltr_int_get_camera_update == NULL){
    if((libhandle = ltr_int_load_library(libname, functions)) != NULL){
      return -1;
    }
  }
  return fun_ltr_int_get_camera_update(heading, pitch, roll, tx, ty, tz);
}

int ltr_suspend(void)
{
  if(fun_ltr_int_suspend == NULL){
    if((libhandle = ltr_int_load_library(libname, functions)) == NULL){
      return -1;
    }
  }
  return fun_ltr_int_suspend();
}

int ltr_wakeup(void)
{
  if(fun_ltr_int_wakeup == NULL){
    if((libhandle = ltr_int_load_library(libname, functions)) == NULL){
      return -1;
    }
  }
  return fun_ltr_int_wakeup();
}

int ltr_shutdown(void)
{
  if(fun_ltr_int_shutdown == NULL){
    if((libhandle = ltr_int_load_library(libname, functions)) == NULL){
      return -1;
    }
  }
  int res = fun_ltr_int_shutdown();
  ltr_int_unload_library(libhandle, functions);
  return res;
}

void ltr_recenter(void)
{
  if(fun_ltr_int_recenter == NULL){
    if((libhandle = ltr_int_load_library(libname, functions)) == NULL){
      return;
    }
  }
  return fun_ltr_int_recenter();
}

ltr_state_type ltr_get_tracking_state(void)
{
  if(fun_ltr_int_get_tracking_state == NULL){
    if((libhandle = ltr_int_load_library(libname, functions)) == NULL){
      return STOPPED;
    }
  }
  return fun_ltr_int_get_tracking_state();
}

void ltr_log_message(const char *format, ...)
{
  va_list ap;
  va_start(ap,format);
  ltr_int_valog_message(format, ap);
  va_end(ap);
}
