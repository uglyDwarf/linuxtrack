/*************************************************************
 ****************** CAMERA ABSTRACTION LAYER *****************
 *************************************************************/
#ifdef HAVE_CONFIG_H
  #include "../config.h"
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#include "cal.h"
#include "utils.h"
#include "dyn_load.h"


static dev_interface iface = {
  .device_run = NULL,
  .device_shutdown = NULL,
  .device_suspend = NULL,
  .device_wakeup = NULL
};

static lib_fun_def_t functions[] = {
  {"ltr_int_rl_run", (void*) &iface.device_run},
  {"ltr_int_rl_shutdown", (void*) &iface.device_shutdown},
  {"ltr_int_rl_suspend", (void*) &iface.device_suspend},
  {"ltr_int_rl_wakeup", (void*) &iface.device_wakeup},
  {NULL, NULL}
};

static void *libhandle = NULL;
ltr_state_type ltr_int_cal_device_state = STOPPED;

/************************/
/* function definitions */
/************************/
int ltr_int_cal_run(struct camera_control_block *ccb, frame_callback_fun cbk)
{
  char *libname = NULL;
  assert(ccb != NULL);
  switch (ccb->device.category) {
    case tir:
      libname = "libtir.so";
      break;
    case tir4_camera:
      libname = "libtir4.so";
      break;
    case webcam:
      libname = "libwc.so";
      break;
    case wiimote:
      libname = "libwii.so";
      break;
    default:
      assert(0);
      break;
  }
  
  ltr_int_log_message("Loading library '%s'\n", libname);
  if((libhandle = ltr_int_load_library(libname, functions)) == NULL){
    return -1;
  }
  ltr_int_log_message("run: %p\n", iface.device_run);
  assert(iface.device_run != NULL);
  ltr_int_log_message("Running!\n");
  int res = (iface.device_run)(ccb, cbk);
  //Runloop blocks until shutdown is called
  ltr_int_unload_library(libhandle, functions);
  return res;
}

int ltr_int_cal_shutdown()
{
  assert(iface.device_shutdown != NULL);
  ltr_int_log_message("Closing!\n");
  int res = (iface.device_shutdown)();
  return res;
}

int ltr_int_cal_suspend()
{
  assert(iface.device_suspend != NULL);
  ltr_int_log_message("Suspending!\n");
  return (iface.device_suspend)();
}

int ltr_int_cal_wakeup()
{
  assert(iface.device_wakeup != NULL);
  ltr_int_log_message("Waking!\n");
  return (iface.device_wakeup)();
}

ltr_state_type ltr_int_cal_get_state()
{
  return ltr_int_cal_device_state;
}

void ltr_int_frame_free(struct camera_control_block *ccb,
                struct frame_type *f)
{
  assert(ccb != NULL);
  assert(f->bloblist.blobs != NULL);
  free(f->bloblist.blobs);
  f->bloblist.blobs = NULL;
}

static void blob_print(struct blob_type b)
{
  printf("x: %f\ty: %f\tscore: %d\n", b.x,b.y,b.score);
}

static void bloblist_print(struct bloblist_type bl)
{
  unsigned int i;

  printf("-- start blob --\n");
  for (i=0;i<bl.num_blobs;i++) {
    blob_print((bl.blobs)[i]);
  }
  printf("-- end blob --\n");
}

void ltr_int_frame_print(struct frame_type f)
{
  printf("-- start frame --\n");
  printf("num blobs: %d\n", f.bloblist.num_blobs);
  bloblist_print(f.bloblist);
  /* FIXME: print something for pixels? */
  printf("-- end frame --\n");
}


