#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <linuxtrack.h>
#include <utils.h>
#include <pthread.h>

bool quit_flag = false;
pthread_t reader;

void *kbd_reader(void *param)
{
  (void)param;
  while(!quit_flag){
    switch(fgetc(stdin)){
      case 'p':
        linuxtrack_suspend();
        printf("Suspending!\n");
        break;
      case 'w':
        linuxtrack_wakeup();
        printf("Waking!\n");
        break;
      case 's':
        linuxtrack_shutdown();
        printf("Shutting!\n");
        quit_flag = true;
        return NULL;
        break;
      case 'r':
        linuxtrack_recenter();
        printf("Recentering!\n");
        break;
      default:
        break;
    }
  }
  return NULL;
}


int main(int argc, char *argv[]) {
  char *section = NULL;//"Default [ \nblablabla";
  float heading, pitch, roll;
  float tx, ty, tz;
  unsigned int counter;
  int retval;
  
  if(argc > 1){
    section = argv[1];
  }
  printf("Section: %s\n", section);
  
  retval = linuxtrack_init(section);
  while(linuxtrack_get_tracking_state() == INITIALIZING){
    ltr_int_usleep(333333);
  }
  if (retval != 0) { 
    printf("Error %d detected! Aborting!\n", retval);
    return retval; 
  };  
  printf("Trying to recenter!\n");
  linuxtrack_recenter();
  pthread_create(&reader, NULL, kbd_reader, NULL);
  unsigned int cntr=-1; 
  while (!quit_flag) {
    retval = linuxtrack_get_pose (&heading,&pitch,&roll,
                                  &tx, &ty, &tz, &counter);
    if(retval < 0){
      printf("Problem reading update!\n");
      break;
    }
    if(cntr != counter){
      cntr = counter;
      printf("hdg:%8.3f pitch:%8.3f roll:%8.3f ", heading, pitch, roll);
      printf("tx:%8.3f y:%8.3f z:%8.3f\n", tx, ty, tz);
    }
    ltr_int_usleep(100000);
  }
  pthread_join(reader, NULL);
  return 0;
}

