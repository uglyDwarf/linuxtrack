#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <linuxtrack.h>
#include <utils.h>

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  float heading, pitch, roll;
  float tx, ty, tz;
  unsigned int counter;
  int retval;

  retval = ltr_init("Default");
  ltr_int_usleep(3000000);
  if (retval != 0) { 
    printf("Error %d detected! Aborting!\n", retval);
    return retval; 
  };  
  printf("Trying to recenter!\n");
  ltr_recenter();
  int cntr=0; 
  while (++cntr < 600) {
    retval = ltr_get_camera_update(&heading,&pitch,&roll,
                                  &tx, &ty, &tz, &counter);
    if (retval < 0) {
      printf("Not updated!\n"); 
      ltr_int_usleep(100000);
      continue; 
    }
    printf("heading: %f\tpitch: %f\troll: %f\n", heading, pitch, roll);
    printf("tx: %f\ty: %f\tz: %f\n", tx, ty, tz);
    ltr_int_usleep(100000);
  }
  ltr_suspend();
  printf("Suspended\n");
  sleep(5);
  printf("Waking up\n");
  ltr_wakeup();
  cntr = 0;
  while (++cntr < 600) {
    retval = ltr_get_camera_update(&heading,&pitch,&roll,
                                  &tx, &ty, &tz, &counter);
    if (retval < 0) {
      printf("Not updated!\n"); 
      ltr_int_usleep(100000);
      continue; 
    }
    printf("heading: %f\tpitch: %f\troll: %f\n", heading, pitch, roll);
    printf("tx: %f\ty: %f\tz: %f\n", tx, ty, tz);
    ltr_int_usleep(100000);
  }
  ltr_shutdown();
  return 0;
}
