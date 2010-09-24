#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "../cal.h"
#include "com_proc.h"
#include "ipc_utils.h"
#include "../wii_driver_prefs.h"
#include "../utils.h"
#include "wii_com.h"

int ltr_int_tracker_init(struct camera_control_block *ccb)
{
  (void)ccb;
  ltr_int_log_message("Initializing Wii!\n");
  if(!ltr_int_wii_init_prefs()){
    ltr_int_log_message("Can't initialize Wii preferences...\n");
    return 1;
  }
  
  if(!initWiiCom(false)){
    ltr_int_log_message("Can't initialize communication with Wii...\n");
    return 1;
  }
  resetFrameFlag();
  resumeWii();
  ltr_int_log_message("Init done!\n"); 
  return 0;
}

int ltr_int_tracker_pause()
{
  pauseWii();
  return 0;
}

int ltr_int_tracker_resume()
{
  resumeWii();
  return 0;
}

int ltr_int_tracker_close()
{
  closeWiiCom();
  return 0;
}

int ltr_int_tracker_get_frame(struct camera_control_block *ccb, 
			      struct frame_type *frame)
{
  (void) ccb;
  bool frame_aquired = false;
  frame->width = 1024/2;
  frame->height = 768/2;
//  read_img_processing_prefs();
  while(!frame_aquired){
    if(getFrameFlag()){
      if(frame->bitmap != NULL){
	memcpy(frame->bitmap, getFramePtr(), frame->width * frame->height);
      }
      resetFrameFlag();
    }
    if(haveNewBlobs()){
	frame->bloblist.num_blobs = getBlobs(frame->bloblist.blobs);
	frame_aquired = true;
    }else{
      usleep(5000);
    }
  }
  return 0;
}
