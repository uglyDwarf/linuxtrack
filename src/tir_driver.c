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


pref_id min_blob = NULL;
pref_id max_blob = NULL;
char *storage_path = NULL;

int tir_init(struct camera_control_block *ccb)
{
  assert(ccb != NULL);
  assert(ccb->device.category == tir);
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
  
  tir_change_operating_mode(ccb, ccb->mode);
  if(open_tir(storage_path, false, get_ir_on(ccb->enable_IR_illuminator_LEDS))){
    ccb->state = active;
    get_res_tir(&(ccb->pixel_height), &(ccb->pixel_width));
    return 0;
  }else{
    ccb->state = suspended;
    return -1;
  }
}

int tir_shutdown(struct camera_control_block *ccb)
{
  if(close_tir()){
    ccb->state = suspended;
    return 0;
  }else{
    return -1;
  }
}

int tir_suspend(struct camera_control_block *ccb)
{
  if(pause_tir()){
    ccb->state = suspended;
    return 0;
  }else{
    return -1;
  }
}

int tir_change_operating_mode(struct camera_control_block *ccb,
                             enum cal_operating_mode newmode)
{
  ccb->mode = newmode;
  switch(ccb->mode){
    case operational_1dot:
      break;
    case operational_3dot:
      break;
    default:
      assert(0);
      break;
  } 
  return 0;   
}

int tir_wakeup(struct camera_control_block *ccb)
{
  if(resume_tir()){
    ccb->state = active;
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

int tir_blobs_to_bt(int num_blobs, plist blob_list, struct bloblist_type *blt)
{
  int min = get_int(min_blob);
  int max = get_int(max_blob);

  struct blob_type *bt = my_malloc(sizeof(struct blob_type) * num_blobs);
  blob *b;
  struct blob_type *cal_b;
  iterator i;
  int counter = 0;
  int valid =0;
  int w, h;
  get_res_tir(&w, &h);
  init_iterator(blob_list, &i);
  while((b = (blob*)get_next(&i)) != NULL){
    if((b->score < min) || (b->score > max)){
      continue;
    }
    ++valid;
    if(counter < num_blobs){
      cal_b = &(bt[counter]);
      cal_b->x = (w / 2.0) - b->x;
      cal_b->y = (h / 2.0) - b->y;
      cal_b->y *= 2.0;
      cal_b->score = b->score;
    }
    ++counter;
  }
  if(valid == num_blobs){
    blt->num_blobs = num_blobs;
    blt->blobs = bt;
    return 0;
  }else{
    log_message("Have %d valid blobs, expecting %d!\n", valid, num_blobs);
    blt->num_blobs = 0;
    free(bt);
    blt->blobs = NULL;
    return -1;
  }
}

int tir_get_frame(struct camera_control_block *ccb, struct frame_type *f)
{
  plist blob_list = NULL;
  if(ccb->diag){
    f->bitmap = my_malloc(ccb->pixel_width * ccb->pixel_height);
    memset(f->bitmap, 0, ccb->pixel_width * ccb->pixel_height);
  }else{
    f->bitmap = NULL;
  }
  if(read_blobs_tir(&blob_list, f->bitmap, ccb->pixel_width, ccb->pixel_height) < 0){
    if(blob_list != NULL){
      free_list(blob_list, true);
    }
    if(ccb->diag){
      free(f->bitmap);
      f->bitmap = NULL;
    }
    return -1;
  }
  if(tir_blobs_to_bt(3, blob_list, &(f->bloblist)) != 0){
    //log_message("Wrong!");
  }
  free_list(blob_list, true);
  if(f->bloblist.num_blobs != 3){
    if(ccb->diag){
      free(f->bitmap);
      f->bitmap = NULL;
    }
    return -1;
  }
  //log_message("Valid block with %d blobs\n", f->bloblist.num_blobs);
  return 0;
}

dev_interface tir_interface = {
  .device_init = tir_init,
  .device_shutdown = tir_shutdown,
  .device_suspend = tir_suspend,
  .device_change_operating_mode = tir_change_operating_mode,
  .device_wakeup = tir_wakeup,
  .device_set_good_indication = tir_set_good,
  .device_get_frame = tir_get_frame,
};


