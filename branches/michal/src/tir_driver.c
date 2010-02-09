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
  blt->num_blobs = num_blobs;
  if(valid == num_blobs){
    return 0;
  }else{
    log_message("Have %d valid blobs, expecting %d!\n", valid, num_blobs);
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


enum request_t {CONTINUE, RUN, PAUSE, SHUTDOWN} request;
enum cal_device_state_type tracker_state = STOPPED;
pthread_cond_t state_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t state_mx = PTHREAD_MUTEX_INITIALIZER;



int ltr_cal_run(struct camera_control_block *ccb, frame_callback_fun cbk)
{
  int retval;
  enum request_t my_request;
  struct frame_type frame;
  bool stop_flag = false;
  
  if(tir_init(ccb) != 0){
    return -1;
  }
  frame.bloblist.blobs = my_malloc(sizeof(struct blob_type) * 3);
  frame.bloblist.num_blobs = 3;
  frame.bitmap = NULL;
  
  tracker_state = RUNNING;
//  resume_tir();
  while(1){
    pthread_mutex_lock(&state_mx);
    my_request = request;
    request = CONTINUE;
    pthread_mutex_unlock(&state_mx);
    switch(tracker_state){
      case RUNNING:
        switch(my_request){
          case PAUSE:
            printf("Pause request!\n");
            tracker_state = PAUSED;
            pause_tir();
            break;
          case SHUTDOWN:
            printf("Shutdown request!\n");
            stop_flag = true;
            break;
          default:
            retval = cal_get_frame(ccb, &frame);
            if(cbk(ccb, &frame) < 0){
              stop_flag = true;
            }
            break;
        }
        break;
      case PAUSED:
        pthread_mutex_lock(&state_mx);
        printf("Waiting for unpause!\n");
        while((request == PAUSE) || (request == CONTINUE)){
          pthread_cond_wait(&state_cv, &state_mx);
          printf("blablabla\n");
        }
        my_request = request;
        request = CONTINUE;
        pthread_mutex_unlock(&state_mx);
        printf("Unpaused!\n");
        resume_tir();
        switch(my_request){
          case RUN:
            printf("Unpause request!\n");
            tracker_state = RUNNING;
            break;
          case SHUTDOWN:
            printf("Shutdown request!\n");
            stop_flag = true;
            break;
          default:
            assert(0);
            break;
        }
        break;
      default:
        assert(0);
        break;
    }
    if(stop_flag == true){
      break;
    }
  }
  
  printf("Thread stopping\n");
  close_tir();
  frame_free(ccb, &frame);
  printf("Thread almost stopped\n");
  return 0;
}

int signal_request(enum request_t req)
{
  pthread_mutex_lock(&state_mx);
  request = req;
  pthread_cond_broadcast(&state_cv);
  pthread_mutex_unlock(&state_mx);
  return 0;
}

int ltr_cal_shutdown()
{
  return signal_request(SHUTDOWN);
}

int ltr_cal_suspend()
{
  return signal_request(PAUSE);
}

int ltr_cal_wakeup()
{
  printf("Requesting wakeup!\n");
  return signal_request(RUN);
}

enum cal_device_state_type ltr_cal_get_state()
{
  return tracker_state;
}

