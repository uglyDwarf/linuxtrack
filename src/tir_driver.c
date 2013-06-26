#include <stdlib.h>
#include <assert.h>
#include "tir_driver.h"
#include "tir_img.h"
#include "tir_hw.h"
#include "list.h"
#include "cal.h"
#include <stdio.h>
#include <string.h>
#include "pref_global.h"
#include "runloop.h"
#include "image_process.h"
#include "usb_ifc.h"
#include "dyn_load.h"
#include "utils.h"
#include "tir_driver_prefs.h"

init_usb_fun *ltr_int_init_usb = NULL;
find_tir_fun *ltr_int_find_tir = NULL;
prepare_device_fun *ltr_int_prepare_device = NULL;
send_data_fun *ltr_int_send_data = NULL;
receive_data_fun *ltr_int_receive_data = NULL;
finish_usb_fun *ltr_int_finish_usb = NULL;

static lib_fun_def_t functions[] = {
  {(char *)"ltr_int_init_usb", (void*) &ltr_int_init_usb},
  {(char *)"ltr_int_find_tir", (void*) &ltr_int_find_tir},
  {(char *)"ltr_int_prepare_device", (void*) &ltr_int_prepare_device},
  {(char *)"ltr_int_send_data", (void*) &ltr_int_send_data},
  {(char *)"ltr_int_receive_data", (void*) &ltr_int_receive_data},
  {(char *)"ltr_int_finish_usb", (void*) &ltr_int_finish_usb},
  {NULL, NULL}
};
static void *libhandle = NULL;
static bool filter_blobs = false;

void flag_pref_changed(void *flag_ptr)
{
  *(bool*)flag_ptr = true;
}

static int last_threshold = -1;

int ltr_int_tracker_init(struct camera_control_block *ccb)
{
  ltr_int_log_message("Initializing the tracker.\n");
  assert(ccb != NULL);
  assert((ccb->device.category == tir) || (ccb->device.category == tir_open));
  last_threshold = -1;
  char *libname = NULL;
  dbg_flag_type fakeusb_dbg_flag = ltr_int_get_dbg_flag('f');
  if(fakeusb_dbg_flag == DBG_ON){
    libname = "libfakeusb";
  }else{
    libname = "libltusb1";
  }
  if((libhandle = ltr_int_load_library(libname, functions)) == NULL){
    ltr_int_log_message("Problem loading library %s!\n", libname);
    return -1;
  }
  if(!ltr_int_tir_init_prefs()){
    ltr_int_log_message("Problem initializing TrackIr prefs!\n");
    return -1;
  }

  ltr_int_log_message("Lib loaded, prefs read...\n");
  if(ltr_int_open_tir(false, !ltr_int_is_model_active())){
    tir_info info;
    ltr_int_get_tir_info(&info);
    ccb->pixel_width = info.width;
    ccb->pixel_height = info.height;
    ltr_int_prepare_for_processing(ccb->pixel_width, ccb->pixel_height);
    switch(info.dev_type){
      case TIR5:
      case TIR5V2:
      case SMARTNAV4:
        filter_blobs = false;
        break;
      default:
        filter_blobs = true;
        break;
    }
    return 0;
  }else{
    return -1;
  }
}

/*
static int tir_set_good(struct camera_control_block *ccb, bool arg)
{
  switch_green(arg);
  return 0;
}
*/

int ltr_int_tracker_get_frame(struct camera_control_block *ccb, struct frame_type *f,
                              bool *frame_acquired)
{
  (void) ccb;
  tir_info info;
  //unsigned int w,h;
  //float hf;
  ltr_int_get_tir_info(&info);
  
  f->width = info.width;
  f->height = info.height;
  f->filter_blobs = filter_blobs;
  image img = {
    .bitmap = f->bitmap,
    .w = info.width,
    .h = info.height,
    .ratio = info.hf
  };
  
  //Set threshold only when needed
  int tmp_thr = ltr_int_tir_get_threshold();
  if(last_threshold != tmp_thr){
    last_threshold = tmp_thr;
    ltr_int_set_threshold_tir(tmp_thr);
  }
  int res = ltr_int_read_blobs_tir(&(f->bloblist), ltr_int_tir_get_min_blob(), 
				ltr_int_tir_get_max_blob(), &img, &info);
  *frame_acquired = true;
  return res;
}

int ltr_int_tracker_pause()
{
  return ltr_int_pause_tir() ? 0 : -1;
}

int ltr_int_tracker_resume()
{
  return ltr_int_resume_tir() ? 0 : -1;
}

int ltr_int_tracker_close()
{
  int res = ltr_int_close_tir() ? 0 : -1;;
  ltr_int_cleanup_after_processing();
  ltr_int_unload_library(libhandle, functions);
  libhandle = NULL;
  return res;
}

int ltr_int_tir_found(bool *have_firmware, bool *have_permissions)
{
  char *libname = NULL;
  dbg_flag_type fakeusb_dbg_flag = ltr_int_get_dbg_flag('f');
  if(fakeusb_dbg_flag == DBG_ON){
    libname = "libfakeusb";
    ltr_int_log_message("Loading fakeusb!\n");
  }else{
    libname = "libltusb1";
  }
  if((libhandle = ltr_int_load_library(libname, functions)) == NULL){
    ltr_int_log_message("Failed to load the library '%s'! \n", libname);
    return 0;
  }
  if(!ltr_int_init_usb()){
    ltr_int_log_message("Failed to initialize usb!\n");
    return 0;
  }
  dev_found device = ltr_int_find_tir();
  printf("Found device %X\n", device);
  if(device & NOT_PERMITTED){
    device ^= NOT_PERMITTED;
    *have_permissions = false;
  }else{
    *have_permissions = true;
  }
  if(device < TIR4){
    *have_firmware = true;
  }else{
    char *fw = ltr_int_find_firmware(device);
    if(fw != NULL){
      free(fw);
      *have_firmware = true;
    }else{
      *have_firmware = false;
    }
  }
  ltr_int_finish_usb(-1);
  ltr_int_unload_library(libhandle, functions);
  libhandle = NULL;
  return device;
}

