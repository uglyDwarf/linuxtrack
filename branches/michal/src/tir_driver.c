#include <stdlib.h>
#include <assert.h>
#include "tir.h"
#include "tir_driver.h"
#include "tir_img.h"
#include "list.h"
#include "cal.h"
#include <stdio.h>
#include <string.h>
#include "pref.h"
#include "pref_int.h"
#include "pref_global.h"
#include "runloop.h"
#include "image_process.h"

tracker_interface trck_iface = {
  .tracker_init = tir_init,
  .tracker_pause = tir_pause,
  .tracker_get_frame = tir_get_frame,
  .tracker_resume = tir_resume,
  .tracker_close = tir_close
};

pref_id min_blob = NULL;
pref_id max_blob = NULL;
char *storage_path = NULL;


int tir_get_prefs()
{
  char *dev_section = get_device_section();
  if(dev_section == NULL){
    return -1;
  }
  if(!open_pref(dev_section, "Max-blob", &max_blob)){
    return -1;
  }
  if(!open_pref(dev_section, "Min-blob", &min_blob)){
    return -1;
  }
  storage_path = get_storage_path();
  
  if(get_int(max_blob) == 0){
    log_message("Please set 'Max-blob' in section %s!\n", dev_section);
    if(!set_int(&max_blob, 200)){
      log_message("Can't set Max-blob!\n");
      return -1;
    }
  }
  if(get_int(min_blob) == 0){
    log_message("Please set 'Min-blob' in section %s!\n", dev_section);
    if(!set_int(&min_blob, 200)){
      log_message("Can't set Min-blob!\n");
      return -1;
    }
  }
  return 0;
}

int tir_init(struct camera_control_block *ccb)
{
  assert(ccb != NULL);
  assert((ccb->device.category == tir) || (ccb->device.category == tir_open));
  tir_get_prefs();
  if(open_tir(storage_path, false, get_ir_on())){
    float tf;
    get_res_tir(&(ccb->pixel_width), &(ccb->pixel_height), &tf);
    prepare_for_processing(ccb->pixel_width, ccb->pixel_height);
    return 0;
  }else{
    return -1;
  }
}

int tir_set_good(struct camera_control_block *ccb, bool arg)
{
  switch_green(arg);
  return 0;
}


int tir_get_frame(struct camera_control_block *ccb, struct frame_type *f)
{
  unsigned int w,h;
  float hf;
  get_res_tir(&w, &h, &hf);
  
  f->width = w;
  f->height = h;
  image img = {
    .bitmap = f->bitmap,
    .w = w,
    .h = h,
    .ratio = hf
  };
  return read_blobs_tir(&(f->bloblist), get_int(min_blob), get_int(max_blob), &img);
}

int tir_pause()
{
  return pause_tir() ? 0 : -1;
}

int tir_resume()
{
  return resume_tir() ? 0 : -1;
}

int tir_close()
{
  return close_tir() ? 0 : -1;
}

