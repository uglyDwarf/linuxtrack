#ifndef WIIMOTE_DRIVER__H
#define WIIMOTE_DRIVER__H

#include <stdbool.h>
#include "cal.h"

/********************/
/* public constants */
/********************/

/* image width and height */ 
#define WIIMOTE_HORIZONTAL_RESOLUTION 1024
#define WIIMOTE_VERTICAL_RESOLUTION 768

/*********************/
/* public data types */
/*********************/
/* none */

extern dev_interface wiimote_interface;

/******************************/
/* public function prototypes */
/******************************/
/* returns true if the wiimote device can be located on 
 * the usb bus */
bool wiimote_is_device_present(struct cal_device_type *cal_device);

/* call to init an uninitialized wiimote device 
 * typically called once at setup
 * turns the IR leds on
 * this function may block for up to 3 seconds 
 * a return value < 0 indicates error */
int wiimote_init(struct camera_control_block *ccb);

/* call to shutdown the wiimote device 
 * typically called once at close
 * turns the IR leds off
 * can be used to deactivate the wiimote;
 * must call init to restart
 * a return value < 0 indicates error */
int wiimote_shutdown(struct camera_control_block *ccb);

/* turn off all the leds, and flush the queue 
 * a return value < 0 indicates error */
int wiimote_suspend(struct camera_control_block *ccb);

/* may only be called while suspended.  Used to change from 
 * operational mode mode to diagnostic mode and vice versa */
int wiimote_change_operating_mode(struct camera_control_block *ccb,
                                enum cal_operating_mode newmode);

/* unsuspend the currently suspended (and inited) 
 * camera device. 
 * IR leds will reactivate, but that is all
 * a return value < 0 indicates error */
int wiimote_wakeup(struct camera_control_block *ccb);

/* read the usb, and process it into frames
 * a return value < 0 indicates error */
int wiimote_get_frame(struct camera_control_block *ccb,
                   struct frame_type *f);

#endif
