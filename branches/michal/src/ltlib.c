#include <stdlib.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include "ltlib_int.h"
#include "utils.h"

int (*fun_lt_int_init)(char *cust_section) = NULL;
int (*fun_lt_int_shutdown)(void) = NULL;
int (*fun_lt_int_suspend)(void) = NULL;
int (*fun_lt_int_wakeup)(void) = NULL;
void (*fun_lt_int_recenter)(void) = NULL;
int (*fun_lt_int_get_camera_update)(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz) = NULL;
bool (*fun_lt_int_open_pref)(char *key, pref_id *prf) = NULL;
bool (*fun_lt_int_create_pref)(char *key) = NULL;
float (*fun_lt_int_get_flt)(pref_id prf) = NULL;
int (*fun_lt_int_get_int)(pref_id prf) = NULL;
char *(*fun_lt_int_get_str)(pref_id prf) = NULL;
bool (*fun_lt_int_set_flt)(pref_id *prf, float f) = NULL;
bool (*fun_lt_int_set_int)(pref_id *prf, int i) = NULL;
bool (*fun_lt_int_set_str)(pref_id *prf, char *str) = NULL;
bool (*fun_lt_int_save_prefs)(void) = NULL;
bool (*fun_lt_int_pref_changed)(pref_id prf) = NULL;
bool (*fun_lt_int_close_pref)(pref_id *prf) = NULL;

void (*fun_lt_int_log_message)(const char *format, ...) = NULL;

typedef struct{
  char *name;
  void *ref;
} lib_fun_def_t;

lib_fun_def_t functions[] = {
{"lt_int_init", (void*) &fun_lt_int_init},
{"lt_int_shutdown", (void*) &fun_lt_int_shutdown},
{"lt_int_suspend", (void*) &fun_lt_int_suspend},
{"lt_int_wakeup", (void*) &fun_lt_int_wakeup},
{"lt_int_recenter", (void*) &fun_lt_int_recenter},
{"lt_int_get_camera_update", (void*) &fun_lt_int_get_camera_update},
{"lt_int_open_pref", (void*) &fun_lt_int_open_pref},
{"lt_int_create_pref", (void*) &fun_lt_int_create_pref},
{"lt_int_get_flt", (void*) &fun_lt_int_get_flt},
{"lt_int_get_int", (void*) &fun_lt_int_get_int},
{"lt_int_get_str", (void*) &fun_lt_int_get_str},
{"lt_int_set_flt", (void*) &fun_lt_int_set_flt},
{"lt_int_set_int", (void*) &fun_lt_int_set_int},
{"lt_int_set_str", (void*) &fun_lt_int_set_str},
{"lt_int_save_prefs", (void*) &fun_lt_int_save_prefs},
{"lt_int_pref_changed", (void*) &fun_lt_int_pref_changed},
{"lt_int_close_pref", (void*) &fun_lt_int_close_pref},
{"lt_int_log_message", (void*) &fun_lt_int_log_message},
{NULL, NULL}
};

static int lt_load_functions()
{
  void *libhandle = NULL;
  
  libhandle = dlopen("liblinuxtrack.so", RTLD_NOW | RTLD_LOCAL);
  if(libhandle == NULL){
    printf("Couldn't load library %s - %s!\n", "liblinuxtrack.so", dlerror());
    return -1;
  }
  dlerror(); //clear any existing error...
  
  int i = 0;
  while(functions[i].name != NULL){
    if((*(void **) (functions[i].ref) = dlsym(libhandle, functions[i].name)) == NULL){
      log_message("Error loding functions: %s\n", dlerror());
      return -1;
    }
    i++;
  }
  /*
  *(void**) (&fun_lt_int_init) = dlsym(libhandle, "lt_int_init");
  *(void**) (&fun_lt_int_shutdown) = dlsym(libhandle, "lt_int_shutdown");
  *(void**) (&fun_lt_int_suspend) = dlsym(libhandle, "lt_int_suspend");
  *(void**) (&fun_lt_int_wakeup) = dlsym(libhandle, "lt_int_wakeup");
  *(void**) (&fun_lt_int_recenter) = dlsym(libhandle, "lt_int_recenter");
  *(void**) (&fun_lt_int_get_camera_update) = dlsym(libhandle, "lt_int_get_camera_update");
  *(void**) (&fun_lt_int_open_pref) = dlsym(libhandle, "lt_int_open_pref");
  *(void**) (&fun_lt_int_create_pref) = dlsym(libhandle, "lt_int_create_pref");
  *(void**) (&fun_lt_int_get_flt) = dlsym(libhandle, "lt_int_get_flt");
  *(void**) (&fun_lt_int_get_int) = dlsym(libhandle, "lt_int_get_int");
  *(void**) (&fun_lt_int_get_str) = dlsym(libhandle, "lt_int_get_str");
  *(void**) (&fun_lt_int_set_flt) = dlsym(libhandle, "lt_int_set_flt");
  *(void**) (&fun_lt_int_set_int) = dlsym(libhandle, "lt_int_set_int");
  *(void**) (&fun_lt_int_set_str) = dlsym(libhandle, "lt_int_set_str");
  *(void**) (&fun_lt_int_save_prefs) = dlsym(libhandle, "lt_int_save_prefs");
  *(void**) (&fun_lt_int_pref_changed) = dlsym(libhandle, "lt_int_pref_changed");
  *(void**) (&fun_lt_int_close_pref) = dlsym(libhandle, "lt_int_close_pref");
  *(void**) (&fun_lt_int_log_message) = dlsym(libhandle, "lt_int_log_message");
  */
  return 0;
}


int lt_init(char *cust_section)
{
  if(fun_lt_int_init == NULL){
    if(lt_load_functions() != 0){
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
    if(lt_load_functions() != 0){
      return -1;
    }
  }
  return fun_lt_int_get_camera_update(heading, pitch, roll, tx, ty, tz);
}

int lt_suspend(void)
{
  if(fun_lt_int_suspend == NULL){
    if(lt_load_functions() != 0){
      return -1;
    }
  }
  return fun_lt_int_suspend();
}

int lt_wakeup(void)
{
  if(fun_lt_int_wakeup == NULL){
    if(lt_load_functions() != 0){
      return -1;
    }
  }
  return fun_lt_int_wakeup();
}

int lt_shutdown(void)
{
  if(fun_lt_int_shutdown == NULL){
    if(lt_load_functions() != 0){
      return -1;
    }
  }
  return fun_lt_int_shutdown();
}

void lt_recenter(void)
{
  if(fun_lt_int_recenter == NULL){
    if(lt_load_functions() != 0){
      return;
    }
  }
  return fun_lt_int_recenter();
}

bool lt_create_pref(char *key)
{
  if(fun_lt_int_create_pref == NULL){
    if(lt_load_functions() != 0){
      return false;
    }
  }
  return fun_lt_int_create_pref(key);
}

bool lt_open_pref(char *key, pref_id *prf)
{
  if(fun_lt_int_open_pref == NULL){
    if(lt_load_functions() != 0){
      return false;
    }
  }
  return fun_lt_int_open_pref(key, prf);
}

float lt_get_flt(pref_id prf)
{
  if(fun_lt_int_get_flt == NULL){
    if(lt_load_functions() != 0){
      return -1.0;
    }
  }
  return fun_lt_int_get_flt(prf);
}

int lt_get_int(pref_id prf)
{
  if(fun_lt_int_get_int == NULL){
    if(lt_load_functions() != 0){
      return -1;
    }
  }
  return fun_lt_int_get_int(prf);
}

char *lt_get_str(pref_id prf)
{
  if(fun_lt_int_get_str == NULL){
    if(lt_load_functions() != 0){
      return NULL;
    }
  }
  return fun_lt_int_get_str(prf);
}

bool lt_set_flt(pref_id *prf, float f)
{
  if(fun_lt_int_set_flt== NULL){
    if(lt_load_functions() != 0){
      return false;
    }
  }
  return fun_lt_int_set_flt(prf, f);
}

bool lt_set_int(pref_id *prf, int i)
{
  if(fun_lt_int_set_int == NULL){
    if(lt_load_functions() != 0){
      return false;
    }
  }
  return fun_lt_int_set_int(prf, i);
}

bool lt_set_str(pref_id *prf, char *str)
{
  if(fun_lt_int_set_str == NULL){
    if(lt_load_functions() != 0){
      return -1;
    }
  }
  return fun_lt_int_set_str(prf, str);
}

bool lt_save_prefs()
{
  if(fun_lt_int_save_prefs == NULL){
    if(lt_load_functions() != 0){
      return false;
    }
  }
  return fun_lt_int_save_prefs();
}

bool lt_pref_changed(pref_id pref)
{
  if(fun_lt_int_pref_changed == NULL){
    if(lt_load_functions() != 0){
      return false;
    }
  }
  return fun_lt_int_pref_changed(pref);
}

bool lt_close_pref(pref_id *prf)
{
  if(fun_lt_int_close_pref == NULL){
    if(lt_load_functions() != 0){
      return false;
    }
  }
  return fun_lt_int_close_pref(prf);
}

void lt_log_message(const char *format, ...)
{
  if(fun_lt_int_log_message == NULL){
    if(lt_load_functions() != 0){
      return;
    }
  }
  va_list ap;
  va_start(ap,format);
  fun_lt_int_log_message(format, ap);
  va_end(ap);
}
