
#include "webcam_driver.h"
#include "utils.h"
#include "string.h"

#include "cal.h"

#include <stdio.h>
#include <unistd.h>

struct camera_control_block ccb;

int main(int argc, char *argv[])
{
  ccb.device.category = webcam;
  ccb.device.device_id = "Live! Cam Optia";
//  ccb.device.device_id = "USB2.0 1.3M UVC WebCam ";
  ccb.mode = operational_3dot;
  ccb.state = pre_init;
  if(webcam_init(&ccb)!= 0)
  {
    printf("Problem initializing webcam!\n");
    return 1;
  };
  printf("Init successfull! Res %d x %d\n", ccb.pixel_width, ccb.pixel_height);
  struct frame_type ft;
  int i;
  
  printf("Reading frames: ");
  for(i = 0; i< 5; ++i){
    if(webcam_get_frame(&ccb, &ft) == 0){
      printf("."); 
    }else{
      printf("Problem getting frame\n");
    }
    frame_free(&ccb, &ft);
  }
  printf("\n");
  printf("Pausing for a 5 seconds...\n");
  if(webcam_suspend(&ccb) != 0){
    printf("Problem suspending!\n");
  }
  sleep(5);
  printf("Starting again...\n");
  if(webcam_wakeup(&ccb) != 0){
    printf("Problem waking up!\n");
  }
  printf("Reading frames: ");
  for(i = 0; i< 5; ++i){
    if(webcam_get_frame(&ccb, &ft) == 0){
      printf("."); 
    }else{
      printf("Problem getting frame\n");
    }
    frame_free(&ccb, &ft);
  }
  printf("\n");
  printf("Shutting down...\n");
  webcam_shutdown(&ccb);
  printf("Webcam closed!\n");
  return 0;
}
