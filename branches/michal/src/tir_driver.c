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

const char *storage_path = NULL;

init_usb_fun ltr_int_init_usb = NULL;
find_tir_fun ltr_int_find_tir = NULL;
prepare_device_fun ltr_int_prepare_device = NULL;
send_data_fun ltr_int_send_data = NULL;
receive_data_fun ltr_int_receive_data = NULL;
finish_usb_fun ltr_int_finish_usb = NULL;

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


void flag_pref_changed(void *flag_ptr)
{
  *(bool*)flag_ptr = true;
}

static int last_threshold = -1;

int ltr_int_tracker_init(struct camera_control_block *ccb)
{
  assert(ccb != NULL);
  assert((ccb->device.category == tir) || (ccb->device.category == tir_open));
  last_threshold = -1;
  if((libhandle = ltr_int_load_library((char *)"libltusb1", functions)) == NULL){
    return -1;
  }
  if(!ltr_int_tir_init_prefs()){
    return -1;
  }
  storage_path = ltr_int_get_data_path("");

  ltr_int_log_message("Lib loaded, prefs read...\n");
  if(ltr_int_open_tir(storage_path, false, !ltr_int_is_model_active())){
    float tf;
    ltr_int_get_res_tir(&(ccb->pixel_width), &(ccb->pixel_height), &tf);
    ltr_int_prepare_for_processing(ccb->pixel_width, ccb->pixel_height);
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

int ltr_int_tracker_get_frame(struct camera_control_block *ccb, struct frame_type *f)
{
  (void) ccb;
  unsigned int w,h;
  float hf;
  ltr_int_get_res_tir(&w, &h, &hf);
  
  f->width = w;
  f->height = h;
  image img = {
    .bitmap = f->bitmap,
    .w = w,
    .h = h,
    .ratio = hf
  };
  
  //Set threshold only when needed
  int tmp_thr = ltr_int_tir_get_threshold();
  if(last_threshold != tmp_thr){
    last_threshold = tmp_thr;
    ltr_int_set_threshold_tir(tmp_thr);
  }
  
  return ltr_int_read_blobs_tir(&(f->bloblist), ltr_int_tir_get_min_blob(), 
				ltr_int_tir_get_max_blob(), &img);
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
  ltr_int_unload_library(libhandle, functions);
  libhandle = NULL;
  return res;
}

int ltr_int_tir_found()
{
  if((libhandle = ltr_int_load_library((char *)"libltusb1", functions)) == NULL){
    return 0;
  }
  if(!ltr_int_init_usb()){
    return 0;
  }
  int res = 0;
  switch(ltr_int_find_tir()){
    case TIR4:
      res = 4;
      break;
    case TIR5:
    case TIR5V2:
      res = 5;
      break;
    default:
      res = 0;
      break;
  }
  ltr_int_finish_usb(-1);
  ltr_int_unload_library(libhandle, functions);
  libhandle = NULL;
  return res;
}
