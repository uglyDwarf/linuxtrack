#include <stdlib.h>
#include <assert.h>
#include "tir.h"
#include "tir_driver.h"
#include "tir_img.h"
#include "list.h"
#include "cal.h"
#include <stdio.h>
int tir_init(struct camera_control_block *ccb)
{
  assert(ccb != NULL);
  assert(ccb->device.category == tir);
  
  tir_change_operating_mode(ccb, ccb->mode);
  
  if(open_tir(".", false, true)){
    ccb->state = active;
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
    case diagnostic:
      break;
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
  struct blob_type *bt = my_malloc(sizeof(struct blob_type) * num_blobs);
  blob *b;
  struct blob_type *cal_b;
  iterator i;
  int counter = 0;
  int w, h;
  get_res_tir(&w, &h);
  init_iterator(blob_list, &i);
  while((b = (blob*)get_next(&i)) != NULL){
    cal_b = &(bt[counter]);
    cal_b->x = (w / 2.0) - b->x;
    cal_b->y = (h / 2.0) - b->y;
    cal_b->score = b->score;
    ++counter;
  }
  blt->num_blobs = num_blobs;
  blt->blobs = bt;
  return 0;
}

int tir_get_frame(struct camera_control_block *ccb, struct frame_type *f)
{
  plist blob_list = NULL;
  int have_blobs = read_blobs_tir(&blob_list);
  if( have_blobs != 3){
    return -1;
  }
  
  tir_blobs_to_bt(3, blob_list, &(f->bloblist));
  free_list(blob_list, true);
  f->bitmap = NULL;
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


