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

  struct blob_type *bt = blt->blobs;
  blob *b;
  struct blob_type *cal_b;
  iterator i;
  int counter = 0;
  int valid =0;
  int w, h;
  float hf;
  get_res_tir(&w, &h, &hf);
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
      cal_b->score = b->score;
    }
    ++counter;
  }
  if(valid == num_blobs){
    blt->num_blobs = num_blobs;
    return 0;
  }else{
    log_message("Have %d valid blobs, expecting %d!\n", valid, num_blobs);
    blt->num_blobs = valid;
    return -1;
  }
}

int cal_get_frame(struct camera_control_block *ccb, struct frame_type *f)
{
  plist blob_list = NULL;
  unsigned int w,h;
  float hf;
  get_res_tir(&w, &h, &hf);
  if(ccb->diag){
    assert(f->bitmap != NULL);
    memset(f->bitmap, 0, w * h);
  }
  
  if(read_blobs_tir(&blob_list, f->bitmap, w, h, hf) < 0){
    if(blob_list != NULL){
      free_list(blob_list, true);
    }
    return -1;
  }
  int res = tir_blobs_to_bt(3, blob_list, &(f->bloblist));
  free_list(blob_list, true);
  return res; 
}

enum cal_device_state_type tracker_state = NOT_INITIALIZED;

int tir_run(struct camera_control_block *ccb, frame_callback_fun cbk)
{
  int retval;
  enum cal_device_state_type last_state;
  struct frame_type frame;
  
  pthread_cond_t state_cv;
  pthread_mutex_t state_mx;
  if(pthread_mutex_init(&state_mx, NULL)){
    fprintf(stderr, "Can't init mutex!\n");
    return 1;
  }
  if(pthread_cond_init(&state_cv, NULL)){
    fprintf(stderr, "Can't init cond. var.!\n");
    return 1;
  }
  if(tir_init(ccb) != 0){
    return -1;
  }

  frame.bloblist.blobs = my_malloc(sizeof(struct blob_type) * 3);
  frame.bloblist.num_blobs = 3;
  frame.bitmap = NULL;
  
  last_state = STOPPED;
  while(1){
    pthread_mutex_lock(&state_mx);
    switch(tracker_state){
      case RUNNING:
        pthread_mutex_unlock(&state_mx);
        retval = cal_get_frame(ccb, &frame);
        break;
      case PAUSED:
        if(last_state == RUNNING){
          pause_tir();
        }
        while(tracker_state == PAUSED){
          pthread_cond_wait(&state_cv, &state_mx);
        }
        pthread_mutex_unlock(&state_mx);
        resume_tir();
        break;
      default:
        break;
    }
    if(tracker_state == STOPPED){
      break;
    }
    last_state = tracker_state;
  }
  printf("Thread stopping\n");
  if(last_state == PAUSED){
    resume_tir();
  }
  close_tir();
  frame_free(ccb, &frame);
  pthread_cond_destroy(&state_cv);
  printf("Thread almost stopped\n");
  return 0;
}


enum cal_device_state_type tir_get_state()
{
  return tracker_state;
}

