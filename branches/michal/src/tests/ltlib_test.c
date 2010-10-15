#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <ltlib.h>

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  float heading, pitch, roll;
  float tx, ty, tz;
  int retval;

  retval = ltr_init("default");
  usleep(15000000);
  if (retval != 0) { 
    printf("Error %d detected! Aborting!\n", retval);
    return retval; 
  };  
  ltr_recenter();
  int cntr=0; 
  while (++cntr < 300) {
    retval = ltr_get_camera_update(&heading,&pitch,&roll,
                                  &tx, &ty, &tz);
    if (retval < 0) {
      printf("Not updated!\n"); 
      usleep(90000);
      continue; 
    }
//    printf("heading: %f\tpitch: %f\troll: %f\n", heading, pitch, roll);
//    printf("tx: %f\ty: %f\tz: %f\n", tx, ty, tz);
    usleep(9000);
  }
  ltr_suspend();
  printf("Suspended\n");
  sleep(5);
  printf("Waking up\n");
  ltr_wakeup();
  cntr = 0;
  while (++cntr < 100) {
    retval = ltr_get_camera_update(&heading,&pitch,&roll,
                                  &tx, &ty, &tz);
    if (retval < 0) {
      printf("Not updated!\n"); 
      usleep(90000);
      continue; 
    }
//    printf("heading: %f\tpitch: %f\troll: %f\n", heading, pitch, roll);
//    printf("tx: %f\ty: %f\tz: %f\n", tx, ty, tz);
    usleep(9000);
  }
  ltr_shutdown();
  return 0;
}
