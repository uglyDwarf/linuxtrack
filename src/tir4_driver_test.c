#include <stdio.h>
#include "tir4_driver.h"

int main(int argc, char **argv) {
  tir4_init();
  /* after init, IR leds are on, but that is it */
  while (true) {
    if ('q' == getchar())
      break;
  }
  /* call below sets led green */
  tir4_set_good_indication(true);
  while (true) {
    if ('q' == getchar())
      break;
  }
  /* call below sets led RED */
  tir4_set_good_indication(false);
  while (true) {
    if ('q' == getchar())
      break;
  }
  /* call disconnects, turns all LEDs off */
  tir4_shutdown();
  return 0;
}
