#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <ltlib.h>
#include <utils.h>

/* int main(int argc, char **argv) { */
/*   struct lt_configuration_type ltconf; */
/*   float heading, pitch, roll; */
/*   float tx, ty, tz; */

/*   lt_init(ltconf); */
/*   sleep(2); */
/*   lt_recenter(); */

/*   while (true) { */
/* /\*   int i; *\/ */
/* /\*   for(i=0;i<500;i++){ *\/ */
/*     lt_get_camera_update(&heading,&pitch,&roll, */
/*                          &tx, &ty, &tz); */

/*     printf("heading: %f\tpitch: %f\n", heading, pitch); */
/*     usleep(20000); */
/*   } */
/*   lt_shutdown(); */
/*   return 0; */
/* } */

int main(int argc, char **argv) {
  float heading, pitch, roll;
  float tx, ty, tz;
  int retval;

  retval = lt_init(NULL);
  usleep(1500000);
  if (retval < 0) { 
    printf("Error %d detected! Aborting!\n", retval);
    return retval; 
  };  
  lt_recenter();
  int cntr=0; 
  while (++cntr < 300) {
    retval = lt_get_camera_update(&heading,&pitch,&roll,
                                  &tx, &ty, &tz);
    if (retval < 0) {
      printf("Not updated!\n"); 
      usleep(90000);
      continue; 
    }
    log_message("heading: %f\tpitch: %f\troll: %f\n", heading, pitch, roll);
    log_message("tx: %f\ty: %f\tz: %f\n", tx, ty, tz);
    printf("heading: %f\tpitch: %f\troll: %f\n", heading, pitch, roll);
    printf("tx: %f\ty: %f\tz: %f\n", tx, ty, tz);
    usleep(9000);
  }
  lt_suspend();
  printf("Suspended\n");
  sleep(5);
  printf("Waking up\n");
  lt_wakeup();
  cntr = 0;
  while (++cntr < 100) {
    retval = lt_get_camera_update(&heading,&pitch,&roll,
                                  &tx, &ty, &tz);
    if (retval < 0) {
      printf("Not updated!\n"); 
      usleep(90000);
      continue; 
    }
    log_message("heading: %f\tpitch: %f\troll: %f\n", heading, pitch, roll);
    log_message("tx: %f\ty: %f\tz: %f\n", tx, ty, tz);
    printf("heading: %f\tpitch: %f\troll: %f\n", heading, pitch, roll);
    printf("tx: %f\ty: %f\tz: %f\n", tx, ty, tz);
    usleep(9000);
  }
  lt_shutdown();
  return 0;
}