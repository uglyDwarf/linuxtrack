#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "../cal.h"
#include "com_proc.h"
#include "../ipc_utils.h"
#include "../wc_driver_prefs.h"
#include "../utils.h"

char *args[] = {"./qt_cam", "-c", "Live! Cam Optia", "-x", "352", "-y", "288", "-f", "xxx", NULL};
static int width;
static int height;
static struct mmap_s mmm;


static bool init_capture(char *prog, char *camera, int w, int h, char *fileName)
{
  char res_x[16];
  char res_y[16];
  snprintf(res_x, sizeof(res_x), "%d", w);
  snprintf(res_y, sizeof(res_y), "%d", h);
  if(mmap_file(fileName, get_com_size() + w * h, &mmm)){
    setCommand(&mmm, WAKEUP);
    args[0] = prog;
    args[2] = camera;
    args[4] = res_x;
    args[6] = res_y;
    args[8] = fileName;
    fork_child(args);
    return true;
  }else{
    return false;
  }
}


static bool read_img_processing_prefs()
{
  setThreshold(&mmm, ltr_int_wc_get_threshold());
  setMinBlob(&mmm, ltr_int_wc_get_min_blob());
  setMaxBlob(&mmm, ltr_int_wc_get_max_blob());
  return true;
}

int ltr_int_tracker_init(struct camera_control_block *ccb)
{
  (void)ccb;
  if(!ltr_int_wc_init_prefs()){
    return -1;
  }
  
  ltr_int_wc_get_resolution(&width, &height);
  
  char *cap_path = ltr_int_get_app_path("/../helper/sg_cam");
  char *cam_id = ltr_int_my_strdup(ltr_int_wc_get_id());
  if(!init_capture(cap_path, cam_id,  width, height, "/tmp/xxx")){
    free(cap_path);
    return 1;
  }
  free(cap_path);
  free(cam_id);
  read_img_processing_prefs();
  resetFrameFlag(&mmm);
  return 0;
}

int ltr_int_tracker_pause()
{
  setCommand(&mmm, SLEEP);
  return 0;
}

int ltr_int_tracker_resume()
{
  setCommand(&mmm, WAKEUP);
  return 0;
}

int ltr_int_tracker_close()
{
  setCommand(&mmm, STOP);
  wait_child_exit(1000);
  return 0;
}

int ltr_int_tracker_get_frame(struct camera_control_block *ccb, 
			      struct frame_type *frame)
{
  (void) ccb;
  bool frame_aquired = false;
  frame->width = width;
  frame->height = height;
  read_img_processing_prefs();
  while(!frame_aquired){
    if(getFrameFlag(&mmm)){
      if(frame->bitmap != NULL){
	memcpy(frame->bitmap, getFramePtr(&mmm), frame->width * frame->height);
      }
      resetFrameFlag(&mmm);
    }
    if(haveNewBlobs(&mmm)){
	frame->bloblist.num_blobs = getBlobs(&mmm, frame->bloblist.blobs);
	frame_aquired = true;
    }else{
      usleep(5000);
    }
  }
  return 0;
}
