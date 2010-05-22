#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <cwiid.h>
#include <unistd.h>
#include "wiimote_driver.h"
#include "image_process.h"
#include "runloop.h"
#include "utils.h"
#include "pref_int.h"
#include "pref_global.h"

/*********************/
/* private Constants */
/*********************/
#define STATE_CHECK_INTERVAL 150

/**********************/
/* private data types */
/**********************/
// Wiimote handler
static cwiid_wiimote_t *gWiimote = NULL;

static int gStateCheckIn = STATE_CHECK_INTERVAL;

static pref_id running_indication = NULL;
static pref_id paused_indication = NULL;
static uint8_t running = CWIID_LED1_ON | CWIID_LED4_ON;
static uint8_t paused = CWIID_LED1_ON;
/*******************************/
/* private function prototypes */
/*******************************/


/************************/
/* function definitions */
/************************/
static void wiimote_refresh_indications(void *param)
{
  param = NULL;
  if(running_indication != NULL){
    char *ind = get_str(running_indication);
    running = 0;
    if(ind[0] == '1') running |= CWIID_LED1_ON;
    if(ind[1] == '1') running |= CWIID_LED2_ON;
    if(ind[2] == '1') running |= CWIID_LED3_ON;
    if(ind[3] == '1') running |= CWIID_LED4_ON;
  }
  if(paused_indication != NULL){
    char *ind = get_str(paused_indication);
    paused = 0;
    if(ind[0] == '1') paused |= CWIID_LED1_ON;
    if(ind[1] == '1') paused |= CWIID_LED2_ON;
    if(ind[2] == '1') paused |= CWIID_LED3_ON;
    if(ind[3] == '1') paused |= CWIID_LED4_ON;
  }
}

static int wiimote_read_indications()
{
  char *sec = get_device_section();
  open_pref_w_callback(sec, "Running-indication", &running_indication,
     wiimote_refresh_indications, NULL);
  open_pref_w_callback(sec, "Paused-indication", &paused_indication,
     wiimote_refresh_indications, NULL);
  wiimote_refresh_indications(NULL);
  return 0;
}



/* call to init an uninitialized wiimote device 
 * typically called once at setup
 * turns the IR leds on
 * this function may block for up to 3 seconds 
 * a return value < 0 indicates error */
int ltr_int_tracker_init(struct camera_control_block *ccb) {
    (void) ccb;
    wiimote_read_indications();
    bdaddr_t bdaddr;
    
    bdaddr = *BDADDR_ANY;
    
    printf("Put Wiimote in discoverable mode now (press 1+2)...\n");
    
    if (!(gWiimote = cwiid_open(&bdaddr, 0))) {
        printf("Wiimote not found\n");
        return -1;
    } else {
        cwiid_set_led(gWiimote, running);
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
int ltr_int_tracker_close() {
    if (gWiimote) cwiid_close(gWiimote);
    return 0;
}

/* turn off all the leds, and flush the queue 
 * a return value < 0 indicates error */
int ltr_int_tracker_pause() {
    cwiid_set_led(gWiimote, paused);
    cwiid_set_rpt_mode(gWiimote, CWIID_RPT_STATUS);
    return 0;
}

/* unsuspend the currently suspended (and inited) 
 * camera device. 
 * IR leds will reactivate, but that is all
 * a return value < 0 indicates error */
int ltr_int_tracker_resume() {
    cwiid_set_led(gWiimote, running);
    cwiid_set_rpt_mode(gWiimote, CWIID_RPT_STATUS | CWIID_RPT_IR);
    return 0;
}

/* read the usb, and process it into frames
 * a return value < 0 indicates error */
int ltr_int_tracker_get_frame(struct camera_control_block *ccb,
                   struct frame_type *f)
{
  (void) ccb;
    struct cwiid_state state;
    unsigned int required_blobnum = 3;
    unsigned int valid;
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
    
    f->width = WIIMOTE_HORIZONTAL_RESOLUTION / 2;
    f->height = WIIMOTE_VERTICAL_RESOLUTION / 2;
    f->bloblist.num_blobs = valid < required_blobnum ? valid : required_blobnum;
    f->bloblist.blobs = (struct blob_type *)
        malloc(f->bloblist.num_blobs*sizeof(struct blob_type));
    assert(f->bloblist.blobs);

    bool draw;
    if(f->bitmap != NULL){
      draw = true;
    }else{
      draw = false;
    }
    image img = {
      .w = WIIMOTE_HORIZONTAL_RESOLUTION / 2,
      .h = WIIMOTE_VERTICAL_RESOLUTION / 2,
      .bitmap = f->bitmap,
      .ratio = 1.0
    };
    valid = 0;
    for (i=0; i<CWIID_IR_SRC_COUNT; i++) {
        if (state.ir_src[i].valid) {
            if (valid<required_blobnum) {
                f->bloblist.blobs[valid].x = -1 * state.ir_src[i].pos[CWIID_X] + WIIMOTE_HORIZONTAL_RESOLUTION/2;
                f->bloblist.blobs[valid].y = state.ir_src[i].pos[CWIID_Y] - WIIMOTE_VERTICAL_RESOLUTION/2;
                f->bloblist.blobs[valid].score = state.ir_src[i].size;
                if(draw){
                  draw_square(&img, state.ir_src[i].pos[CWIID_X] / 2, (WIIMOTE_VERTICAL_RESOLUTION - state.ir_src[i].pos[CWIID_Y]) / 2, 2*state.ir_src[i].size);
                  draw_cross(&img, state.ir_src[i].pos[CWIID_X] / 2, (WIIMOTE_VERTICAL_RESOLUTION - state.ir_src[i].pos[CWIID_Y]) / 2, (int)WIIMOTE_HORIZONTAL_RESOLUTION/100.0);
                }
            }
            valid++;
        }
    }
    return 0;
}
