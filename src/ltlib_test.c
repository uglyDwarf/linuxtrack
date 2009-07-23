#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "ltlib.h"

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
  struct lt_configuration_type ltconf;
  float heading, pitch, roll;
  float tx, ty, tz;
  int retval;

  retval = lt_init(ltconf, NULL);
  if (retval < 0) { 
    printf("Error %d detected! Aborting!\n", retval);
    return retval; 
  };  
  sleep(2);
  lt_recenter();

  while (true) {
    retval = lt_get_camera_update(&heading,&pitch,&roll,
                                  &tx, &ty, &tz);
    if (retval < 0) { 
      printf("Error %d detected! Aborting!\n", retval);
      return retval; 
    };
    printf("heading: %f\tpitch: %f\n", heading, pitch);
    usleep(20000);
  }
  lt_shutdown();
  return 0;
}
