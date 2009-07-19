#include <stdlib.h>
#include <stdio.h>
#include "ltlib.h"

int main(int argc, char **argv) {
  struct lt_configuration_type ltconf;
  float heading, pitch, roll;
  float tx, ty, tz;

  lt_init(ltconf);
  sleep(2);
  lt_recenter();

  while (true) {
    lt_get_camera_update(&heading,&pitch,&roll,
                         &tx, &ty, &tz);

    printf("heading: %f\tpitch: %f\n", heading, pitch);
  }
  return 0;
}
