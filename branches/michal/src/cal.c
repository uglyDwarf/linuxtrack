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


dev_interface iface = {
  .device_run = NULL,
  .device_shutdown = NULL,
  .device_suspend = NULL,
  .device_wakeup = NULL,
  .device_get_state = NULL
};

static lib_fun_def_t functions[] = {
  {"ltr_cal_run", (void*) &iface.device_run},
  {"ltr_cal_shutdown", (void*) &iface.device_shutdown},
  {"ltr_cal_suspend", (void*) &iface.device_suspend},
  {"ltr_cal_wakeup", (void*) &iface.device_wakeup},
  {"ltr_cal_get_state", (void*) &iface.device_get_state},
  {NULL, NULL}
};

void *libhandle = NULL;

/************************/
/* function definitions */
/************************/
int cal_run(struct camera_control_block *ccb, frame_callback_fun cbk)
{
  char *libname = NULL;
  assert(ccb != NULL);
  switch (ccb->device.category) {
    case tir:
      libname = "libtir1.so";
      break;
    case tir_open:
      libname = "libtir2.so";
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
  
  log_message("Loading library '%s'\n", libname);
  if((libhandle = lt_load_library(libname, functions)) == NULL){
    return -1;
  }
  log_message("run: %p\n", iface.device_run);
  assert(iface.device_run != NULL);
  log_message("Running!\n");
  return (iface.device_run)(ccb, cbk);
}

int cal_shutdown()
{
  assert(iface.device_shutdown != NULL);
  log_message("Closing!\n");
  int res = (iface.device_shutdown)();
  lt_unload_library(libhandle, functions);
  return res;
}

int cal_suspend()
{
  assert(iface.device_suspend != NULL);
  log_message("Suspending!\n");
  return (iface.device_suspend)();
}

int cal_wakeup()
{
  assert(iface.device_wakeup != NULL);
  log_message("Waking!\n");
  return (iface.device_wakeup)();
}

enum cal_device_state_type cal_get_state()
{
  assert(iface.device_get_state != NULL);
  return (iface.device_get_state)();
}

void frame_free(struct camera_control_block *ccb,
                struct frame_type *f)
{
  assert(ccb != NULL);
  assert(f->bloblist.blobs != NULL);
  free(f->bloblist.blobs);
  f->bloblist.blobs = NULL;
}

void blob_print(struct blob_type b)
{
  printf("x: %f\ty: %f\tscore: %d\n", b.x,b.y,b.score);
}

void bloblist_print(struct bloblist_type bl)
{
  int i;

  printf("-- start blob --\n");
  for (i=0;i<bl.num_blobs;i++) {
    blob_print((bl.blobs)[i]);
  }
  printf("-- end blob --\n");
}

void frame_print(struct frame_type f)
{
  printf("-- start frame --\n");
  printf("num blobs: %d\n", f.bloblist.num_blobs);
  bloblist_print(f.bloblist);
  /* FIXME: print something for pixels? */
  printf("-- end frame --\n");
}


