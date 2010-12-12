#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "../cal.h"
#include "com_proc.h"
#include "../ipc_utils.h"
#include "../wc_driver_prefs.h"
#include "../utils.h"

const char *args[] = {"./qt_cam", "-c", "Live! Cam Optia", "-x", "352", "-y", "288", "-f", "xxx", NULL};
static int width;
static int height;


static bool init_capture(const char *prog, const char *camera, int w, int h, const char *fileName)
{
  char res_x[16];
  char res_y[16];
  snprintf(res_x, sizeof(res_x), "%d", w);
  snprintf(res_y, sizeof(res_y), "%d", h);
  if(mmap_file(fileName, w * h)){
    setCommand(WAKEUP);
    args[0] = prog;
    args[2] = camera;
    args[4] = res_x;
    args[6] = res_y;
    args[8] = fileName;
    fork_child(0, 0, args);
    return true;
  }else{
    return false;
  }
}


static bool read_img_processing_prefs()
{
  setThreshold(ltr_int_wc_get_threshold());
  setMinBlob(ltr_int_wc_get_min_blob());
  setMaxBlob(ltr_int_wc_get_max_blob());
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
  const char *cam_id = ltr_int_wc_get_id();
  if(!init_capture(cap_path, cam_id,  width, height, "/tmp/xxx")){
    free(cap_path);
    return 1;
  }
  free(cap_path);
  read_img_processing_prefs();
  resetFrameFlag();
  return 0;
}

int ltr_int_tracker_pause()
{
  setCommand(SLEEP);
  return 0;
}

int ltr_int_tracker_resume()
{
  setCommand(WAKEUP);
  return 0;
}

int ltr_int_tracker_close()
{
  setCommand(STOP);
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
