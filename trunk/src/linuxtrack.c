#include <stdio.h>
#include <dlfcn.h>
#include "linuxtrack.h"

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#ifdef DARWIN
static const char *libname = "liblinuxtrack.0.dylib";
#else
static const char *libname = "liblinuxtrack.so.0";
#endif

typedef int (*ltr_gp_t)(void);
typedef int (*ltr_init_t)(const char *cust_section);
typedef int (*ltr_get_camera_update_t)(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz,
                         uint32_t *counter);
typedef ltr_state_type (*ltr_get_tracking_state_t)(void);




static ltr_init_t ltr_init_fun = NULL;
static ltr_gp_t ltr_shutdown_fun = NULL;
static ltr_gp_t ltr_suspend_fun = NULL;
static ltr_gp_t ltr_wakeup_fun = NULL;
static ltr_gp_t ltr_recenter_fun = NULL;
static ltr_get_camera_update_t ltr_get_camera_update_fun = NULL;
static ltr_get_tracking_state_t ltr_get_tracking_state_fun = NULL;

static void *lib_handle = NULL;

struct func_defs_t{
  char *name;
  void *ref;
};

struct func_defs_t functions[] = 
{
  {"ltr_init", (void*)&ltr_init_fun},
  {"ltr_shutdown", (void*)&ltr_shutdown_fun},
  {"ltr_suspend", (void *)&ltr_suspend_fun},
  {"ltr_wakeup", (void *)&ltr_wakeup_fun},
  {"ltr_recenter", (void *)&ltr_recenter_fun},
  {"ltr_get_camera_update", (void *)&ltr_get_camera_update_fun},
  {"ltr_get_tracking_state", (void *)&ltr_get_tracking_state_fun},
  {NULL, NULL}
};

int linuxtrack_shutdown(void)
{
  int res;
  if(ltr_shutdown_fun == NULL){
    return -1;
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
    ltr_get_camera_update_fun = NULL;
    ltr_get_tracking_state_fun = NULL;
    dlclose(handle);
  }
  return res;
}

int linuxtrack_suspend(void)
{
  if(ltr_suspend_fun == NULL){
    return -1;
  }
  return ltr_suspend_fun();
}

int linuxtrack_wakeup(void)
{
  if(ltr_wakeup_fun == NULL){
    return -1;
  }
  return ltr_wakeup_fun();
}

int linuxtrack_recenter(void)
{
  if(ltr_recenter_fun == NULL){
    return -1;
  }
  return ltr_recenter_fun();
}

int linuxtrack_get_pose(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz,
                         uint32_t *counter)
{
  if(ltr_get_camera_update_fun == NULL){
    return -1;
  }
  return ltr_get_camera_update_fun(heading, pitch, roll, tx, ty, tz, counter);
}

ltr_state_type linuxtrack_get_tracking_state(void)
{
  if(ltr_get_tracking_state_fun == NULL){
    return -1;
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
      fprintf(stderr, "Couldn't load symbol '%s': %s\n", (functions[i]).name, dlerror());
      return -1;
    }
    *((void **)(functions[i]).ref) = symbol;
    ++i;
  }
  return 0;
}

static int linuxtrack_load_library()
{
  lib_handle = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
  if(lib_handle == NULL){
    fprintf(stderr, "Couldn't load library '%s' - %s!\n", libname, dlerror());
    return -1;
  }
  dlerror(); /*clear any existing error...*/
  if(linuxtrack_load_functions(lib_handle) != 0){
    return -1;
  }
  return 0;
}

int linuxtrack_init(const char *cust_section)
{
  if(linuxtrack_load_library() != 0){
    return -1;
  }
  return ltr_init_fun(cust_section);
}



