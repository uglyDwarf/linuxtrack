#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <cal.h>
#include "com_proc.h"
#include "ipc_utils.h"
#include "wii_driver_prefs.h"
#include "utils.h"
#include "wii_com.h"

static struct mmap_s *mmm;

static int get_indication()
{
  int indication = 0;
  bool d1, d2, d3, d4;
  ltr_int_get_run_indication(&d1, &d2, &d3, &d4);
  if(d1) indication |= 1;
  if(d2) indication |= 2;
  if(d3) indication |= 4;
  if(d4) indication |= 8;
  indication <<= 4;
  ltr_int_get_pause_indication(&d1, &d2, &d3, &d4);
  if(d1) indication |= 1;
  if(d2) indication |= 2;
  if(d3) indication |= 4;
  if(d4) indication |= 8;
  ltr_int_log_message("Indication: %X\n", indication);
  return indication;
}

int ltr_int_tracker_init(struct camera_control_block *ccb)
{
  (void)ccb;
  ltr_int_log_message("Initializing Wii!\n");
  if(!ltr_int_wii_init_prefs()){
    ltr_int_log_message("Can't initialize Wii preferences...\n");
    return 1;
  }
  
  if(!ltr_int_initWiiCom(false, &mmm)){
    ltr_int_log_message("Can't initialize communication with Wii...\n");
    return 1;
  }
  ltr_int_resetFrameFlag(mmm);
  ltr_int_wii_init_prefs();
  ltr_int_setWiiIndication(mmm, get_indication());
  ltr_int_resumeWii();
  ltr_int_log_message("Init done!\n"); 
  return 0;
}

int ltr_int_tracker_pause()
{  
  ltr_int_setWiiIndication(mmm, get_indication());
  ltr_int_pauseWii();
  return 0;
}

int ltr_int_tracker_resume()
{
  ltr_int_setWiiIndication(mmm, get_indication());
  ltr_int_resumeWii();
  return 0;
}

int ltr_int_tracker_close()
{
  ltr_int_closeWiiCom();
  return 0;
}

int ltr_int_tracker_get_frame(struct camera_control_block *ccb,
			      struct frame_type *frame, bool *frame_acquired)
{
  (void) ccb;
  frame->width = 1024/2;
  frame->height = 768/2;
  if(ltr_int_getFrameFlag(mmm)){
    if(frame->bitmap != NULL){
      memcpy(frame->bitmap, ltr_int_getFramePtr(mmm), frame->width * frame->height);
    }
    ltr_int_resetFrameFlag(mmm);
  }
  if(ltr_int_haveNewBlobs(mmm)){
   frame->bloblist.num_blobs = ltr_int_getBlobs(mmm, frame->bloblist.blobs, frame->bloblist.num_blobs);
   *frame_acquired = true;
  }else{
    ltr_int_usleep(5000);
  }
  return 0;
}
