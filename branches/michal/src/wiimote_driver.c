#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <cwiid.h>
#include <unistd.h>
#include "wiimote_driver.h"
#include "runloop.h"
#include "utils.h"

/*************/
/* interface */
/*************/



tracker_interface trck_iface = {
  .tracker_init = wiimote_init,
  .tracker_pause = wiimote_suspend,
  .tracker_get_frame = wiimote_get_frame,
  .tracker_resume = wiimote_wakeup,
  .tracker_close = wiimote_shutdown
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
    
    printf("Put Wiimote in discoverable mode now (press 1+2)...\n");
    
    if (!(gWiimote = cwiid_open(&bdaddr, 0))) {
        printf("Wiimote not found\n");
        return -1;
    } else {
        cwiid_set_led(gWiimote, CWIID_LED1_ON | CWIID_LED4_ON);
        cwiid_set_rpt_mode(gWiimote, CWIID_RPT_STATUS | CWIID_RPT_IR);
        log_message("Wiimote connected\n");
    }
    return 0;
}

/* call to shutdown the wiimote device 
 * typically called once at close
 * turns the IR leds off
 * can be used to deactivate the wiimote;
 * must call init to restart
 * a return value < 0 indicates error */
int wiimote_shutdown() {
    if (gWiimote) cwiid_close(gWiimote);
    return 0;
}

/* turn off all the leds, and flush the queue 
 * a return value < 0 indicates error */
int wiimote_suspend() {
    cwiid_set_led(gWiimote, CWIID_LED1_ON);
    cwiid_set_rpt_mode(gWiimote, CWIID_RPT_STATUS);
    return 0;
}

/* unsuspend the currently suspended (and inited) 
 * camera device. 
 * IR leds will reactivate, but that is all
 * a return value < 0 indicates error */
int wiimote_wakeup() {
    cwiid_set_led(gWiimote, CWIID_LED1_ON | CWIID_LED4_ON);
    cwiid_set_rpt_mode(gWiimote, CWIID_RPT_STATUS | CWIID_RPT_IR);
    return 0;
}

static void clip_coord(int *coord, int min,
                       int max)
{
  int tmp = *coord;
  tmp = (tmp < min) ? min : tmp;
  tmp = (tmp > max) ? max : tmp;
  *coord = tmp;
}

void draw_cross(struct frame_type *f, int x, int y, int size)
{
  int cntr;
  int x_m = x - size;
  int x_p = x + size;
  int y_m = y - size;
  int y_p = y + size;
  clip_coord(&x_m, 0, f->width);
  clip_coord(&x_p, 0, f->width);
  clip_coord(&y_m, 0, f->height);
  clip_coord(&y_p, 0, f->height);
  
  unsigned char *pt;
  pt = f->bitmap + (f->width * y) + x_m;
  for(cntr = x_m; cntr < x_p; ++cntr){
    *(pt++) = 0xFF;
  }
  pt = f->bitmap + (f->width * y_m) + x;
  for(cntr = y_m; cntr < y_p; ++cntr){
    *pt = 0xFF;
    pt += f->width;
  }
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
    
    //Otherwise the polling takes too much processor
    usleep(10000);

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
    
    f->width = WIIMOTE_HORIZONTAL_RESOLUTION;
    f->height = WIIMOTE_VERTICAL_RESOLUTION;
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
                if(f->bitmap != NULL){
                  draw_cross(f, state.ir_src[i].pos[CWIID_X], state.ir_src[i].pos[CWIID_Y], state.ir_src[i].size);
                }
            }
            valid++;
        }
    }
    return 0;
}
