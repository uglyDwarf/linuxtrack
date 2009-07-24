#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <cwiid.h>
#include "wiimote_driver.h"

/*************/
/* interface */
/*************/

dev_interface wiimote_interface = {
  .device_init = wiimote_init,
  .device_shutdown = wiimote_shutdown,
  .device_suspend = wiimote_suspend,
  .device_change_operating_mode = wiimote_change_operating_mode,
  .device_wakeup = wiimote_wakeup,
  .device_set_good_indication = NULL,
  .device_get_frame = wiimote_get_frame,
};



/*********************/
/* private Constants */
/*********************/
#define STATE_CHECK_INTERVAL 150

/**********************/
/* private data types */
/**********************/
// Wiimote handler
cwiid_wiimote_t *gWiimote = NULL;

int gStateCheckIn = STATE_CHECK_INTERVAL;

/*******************************/
/* private function prototypes */
/*******************************/


/************************/
/* function definitions */
/************************/
/* returns true if the wiimote device can be located on 
 * the usb bus */
bool wiimote_is_device_present(struct cal_device_type *cal_device) {
    return true;
}

/* call to init an uninitialized wiimote device 
 * typically called once at setup
 * turns the IR leds on
 * this function may block for up to 3 seconds 
 * a return value < 0 indicates error */
int wiimote_init(struct camera_control_block *ccb) {
    bdaddr_t bdaddr;
    
    bdaddr = *BDADDR_ANY;
    
    fprintf(stderr, "Put Wiimote in discoverable mode now (press 1+2)...\n");
    
    if (!(gWiimote = cwiid_open(&bdaddr, 0))) {
        fprintf(stderr, "Wiimote not found\n");
        return -1;
    } else {
        cwiid_set_led(gWiimote, CWIID_LED1_ON | CWIID_LED4_ON);
        cwiid_set_rpt_mode(gWiimote, CWIID_RPT_STATUS | CWIID_RPT_IR);
        fprintf(stderr, "Wiimote connected\n");
    }
    return 0;
}

/* call to shutdown the wiimote device 
 * typically called once at close
 * turns the IR leds off
 * can be used to deactivate the wiimote;
 * must call init to restart
 * a return value < 0 indicates error */
int wiimote_shutdown(struct camera_control_block *ccb) {
    if (gWiimote) cwiid_close(gWiimote);
    return 0;
}

/* turn off all the leds, and flush the queue 
 * a return value < 0 indicates error */
int wiimote_suspend(struct camera_control_block *ccb) {
    // TODO - we might turn off IR reception here
    return 0;
}

/* may only be called while suspended.  Used to change from 
 * operational mode mode to diagnostic mode and vice versa */
int wiimote_change_operating_mode(struct camera_control_block *ccb,
                                enum cal_operating_mode newmode)
{
    // TODO
    return 0;
}


/* unsuspend the currently suspended (and inited) 
 * camera device. 
 * IR leds will reactivate, but that is all
 * a return value < 0 indicates error */
int wiimote_wakeup(struct camera_control_block *ccb) {
    // TODO
    return 0;
}

/* this controls the wiimote red and green LED
 * typically called whenever the tracking is "good"
 * when called with true, the green led is lit
 * when called with false, the red led is lit
 * neither is lit immediatly after init!
 * a return value < 0 indicates error */
int wiimote_set_good_indication(struct camera_control_block *ccb,
                             bool arg)
{
    cwiid_set_led(gWiimote, CWIID_LED1_ON | CWIID_LED4_ON);
    return 0;
}

/* read the usb, and process it into frames
 * a return value < 0 indicates error */
int wiimote_get_frame(struct camera_control_block *ccb,
                   struct frame_type *f)
{
    struct cwiid_state state;
    unsigned int required_blobnum = 3;
    int valid;
    int i;


    if (!gStateCheckIn--) {
        gStateCheckIn = STATE_CHECK_INTERVAL;

        if (cwiid_request_status(gWiimote)) {
            fprintf(stderr, "Requesting status failed, disconnecting\n");
            cwiid_close(gWiimote);
            gWiimote = NULL;
            return -1;
        }
    }

    if (cwiid_get_state(gWiimote, &state)) {
        // Treat connection as disconnected on error
        fprintf(stderr, "Error reading wiimote state\n");
        cwiid_close(gWiimote);
        gWiimote = NULL;
        return -1;
    }
    
    valid = 0;
    for (i=0; i<CWIID_IR_SRC_COUNT; i++) {
        if (state.ir_src[i].valid) {
            valid++;
        }
    }
    
    f->bloblist.num_blobs = valid < required_blobnum ? valid : required_blobnum;
    f->bloblist.blobs = (struct blob_type *)
        malloc(f->bloblist.num_blobs*sizeof(struct blob_type));
    assert(f->bloblist.blobs);

    valid = 0;
    for (i=0; i<CWIID_IR_SRC_COUNT; i++) {
        if (state.ir_src[i].valid) {
            if (valid<required_blobnum) {
                f->bloblist.blobs[valid].x = -1 * state.ir_src[i].pos[CWIID_X] + WIIMOTE_HORIZONTAL_RESOLUTION/2;
                f->bloblist.blobs[valid].y = state.ir_src[i].pos[CWIID_Y] - WIIMOTE_VERTICAL_RESOLUTION/2;
                f->bloblist.blobs[valid].score = state.ir_src[i].size;
            }
            valid++;
        }
    }
    return 0;
}
