#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <linuxtrack.h>

static float heading, pitch, roll, x, y, z;
static unsigned int counter;

bool intialize_tracking(void)
{
  linuxtrack_state_type state;
  //Initialize the tracking using Default profile
  state = linuxtrack_init(NULL);
  if(state < LINUXTRACK_OK){
    printf("%s\n", linuxtrack_explain(state));
    return false;
  }
  int timeout = 0;
  //wait up to 20 seconds for the tracker initialization
  while(timeout < 200){
    state = linuxtrack_get_tracking_state();
    printf("Status: %s\n", linuxtrack_explain(state));
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
  int frames = 0;
  
  if(!intialize_tracking()){
    return 1;
  }
  
  //do the tracking
  while(frames < 100){ //100 frames ~ 10 seconds
    if(linuxtrack_get_pose(&heading, &pitch, &roll, &x, &y, &z, &counter) > 0){
      printf("%f  %f  %f\n  %f  %f  %f\n", heading, pitch, roll, x, y, z);
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

