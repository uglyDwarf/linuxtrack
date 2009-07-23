#ifndef TIR4_DRIVER__H
#define TIR4_DRIVER__H

#include <stdbool.h>
#include <usb.h>
#include "cal.h"

/********************/
/* public constants */
/********************/
#define TIR4_VERTICAL_SQUASHED_RESOLUTION 288

/* image width and height */ 
#define TIR4_HORIZONTAL_RESOLUTION 710
#define TIR4_VERTICAL_RESOLUTION (TIR4_VERTICAL_SQUASHED_RESOLUTION * 2)

/*********************/
/* public data types */
/*********************/
/* none */

/******************************/
/* public function prototypes */
/******************************/
/* returns true if the tir4 device can be located on 
 * the usb bus */
bool tir4_is_device_present(struct cal_device_type *cal_device);

/* call to init an uninitialized tir4 device 
 * typically called once at setup
 * turns the IR leds on
 * this function may block for up to 3 seconds 
 * a return value < 0 indicates error */
int tir4_init(struct camera_control_block *ccb);

/* call to shutdown the tir4 device 
 * typically called once at close
 * turns the IR leds off
 * can be used to deactivate the tir4;
 * must call init to restart
 * a return value < 0 indicates error */
int tir4_shutdown(struct camera_control_block *ccb);

/* turn off all the leds, and flush the queue 
 * a return value < 0 indicates error */
int tir4_suspend(struct camera_control_block *ccb);

/* may only be called while suspended.  Used to change from 
 * operational mode mode to diagnostic mode and vice versa */
int tir4_change_operating_mode(struct camera_control_block *ccb,
                               enum cal_operating_mode newmode);

/* unsuspend the currently suspended (and inited) 
 * camera device. 
 * IR leds will reactivate, but that is all
 * a return value < 0 indicates error */
int tir4_wakeup(struct camera_control_block *ccb);

/* this controls the tir4 red and green LED
 * typically called whenever the tracking is "good"
 * when called with true, the green led is lit
 * when called with false, the red led is lit
 * neither is lit immediatly after init!
 * a return value < 0 indicates error */
int tir4_set_good_indication(struct camera_control_block *ccb,
                             bool arg);

/* read the usb, and process it into frames
 * a return value < 0 indicates error */
int tir4_get_frame(struct camera_control_block *ccb,
                   struct frame_type *f);

#endif
