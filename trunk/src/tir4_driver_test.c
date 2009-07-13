#include <stdio.h>
#include "tir4_driver.h"

int main(int argc, char **argv) {
  struct tir4_frame_type frame;

  tir4_init();
  /* after init, IR leds are on, but that is it */

  /* call below sets led green */
  tir4_set_good_indication(true);

  /* loop forever reading and printing results */
  while (true) {
    tir4_do_read();
    while (tir4_frame_is_available()) {
      tir4_get_frame(&frame);
      tir4_frame_print(&frame);
    }
  }

  /* call disconnects, turns all LEDs off */
  tir4_shutdown();
  return 0;
}
