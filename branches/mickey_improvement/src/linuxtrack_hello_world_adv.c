#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <linuxtrack.h>

bool intialize_tracking(void)
{
  //Initialize the tracking using Default profile
  linuxtrack_init(NULL);
  
  int timeout = 0;
  linuxtrack_state_type state;
  //wait up to 20 seconds for the tracker initialization
  while(timeout < 200){
    state = linuxtrack_get_tracking_state();
    if((state == RUNNING) || (state == PAUSED)){
      return true;
    }
    usleep(100000);
    ++timeout;
  }
  printf("Linuxtrack doesn't work right!\n");
  printf("Make sure it is installed and configured correctly.\n");
  return false;
}


int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  printf("Linuxtrack: Hello World!\n");

  if(!intialize_tracking()){
    return 1;
  }

  const int num_blobs = 10;
  linuxtrack_pose_t pose;
  float blobs[3 * num_blobs];
  int read, i;
  int frames = 0;
  //do the tracking
  while(frames < 100){ //100 frames ~ 10 seconds
    if(linuxtrack_get_pose_full(&pose, blobs, num_blobs, &read) > 0){
      //do something with the new pose
      printf("%d: %d %f  %f  %f\n  %f  %f  %f\n", read, pose.counter, pose.yaw, pose.pitch, 
                                               pose.roll, pose.tx, pose.ty, pose.tz);
      printf(" Res: %d x %d\n", pose.resolution_x, pose.resolution_y);
      for(i = 0; i < read*3; i += 3){
        printf("  %d: [%g; %g] (%g)\n", i/3, blobs[i], blobs[i + 1], blobs[i + 2]);
      }
    }
    ++frames;
    if(frames == 40){
      //pause for a bit
      linuxtrack_suspend();
    }else if(frames == 70){
      //resume
      linuxtrack_wakeup();
    }
    //here you'd do some usefull stuff
    usleep(100000);
  }
  
  //stop the tracker
  linuxtrack_shutdown();
  return 0;
}

