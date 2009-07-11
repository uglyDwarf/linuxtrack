#ifndef TIR4_DRIVER__H
#define TIR4_DRIVER__H

#include <stdbool.h>
#include <usb.h>

/* call to init an uninitialized tir4 device 
 * typically called once at setup
 * turns the IR leds on
 * this function may block for up to 1 second */ 
void tir4_init(void);

/* call to shutdown the tir4 device 
 * typically called once at close
 * turns the IR leds off
 * can be used to deactivate the tir4;
 * must call init to restart */
void tir4_shutdown(void);

/* this controls the tir4 red and green LED
 * typically called whenever the tracking is "good"
 * when called with true, the green led is lit
 * when called with false, the red led is lit
 * neither is lit immediatly after init!
 */
void tir4_set_good_indication(bool arg);

#endif
