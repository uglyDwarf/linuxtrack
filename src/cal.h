/*************************************************************
 ****************** CAMERA ABSTRACTION LAYER *****************
 *************************************************************/
#ifndef NEW_CAL__H
#define NEW_CAL__H
#include <stdbool.h>
#include "linuxtrack.h"
#include "ltlib_int.h"

#ifdef __cplusplus
extern "C" {
#endif

enum ltr_request_t {CONTINUE, RUN, PAUSE, SHUTDOWN};

struct blob_type {
  /* coordinates of the blob on the screen
   * these coordinates will have the center of
   * the screen at (0,0).*
   * (+resx/2,+resy/2) = top right corner
   * (-resx/2,-resy/2) = bottom left corner
   */
  float x,y;
  /* total # pixels area, used for sorting/scoring blobs */
  unsigned int score;
};

struct bloblist_type {
  unsigned int num_blobs;
  unsigned int expected_blobs;
  /* array of blobs, they will come from the driver
   * already sorted in area.  The driver will allocate
   * memory for these, and the caller must call
   * frame_free(frame) to free these when finished */
  struct blob_type *blobs;
};

struct frame_type {
  struct bloblist_type bloblist;
  unsigned int width;
  unsigned int height;
  unsigned int counter;
  unsigned char *bitmap; /* 8bits per pixel, monochrome 0x00 or 0xff */
};

typedef enum cal_device_category_type {
  tir4_camera,
  webcam,
  wiimote,
  webcam_ft,
  tir,
  tir_open,
  mac_webcam,
  mac_webcam_ft,
  joystick,
  mac_ps3eye
} cal_device_category_type;

struct cal_device_type {
  enum cal_device_category_type category;
  char *device_id;
};

struct camera_control_block {
  struct cal_device_type device;
  unsigned int pixel_width;
  unsigned int pixel_height;
  bool diag;
  bool enable_IR_illuminator_LEDS; /* for tir4 */
};

typedef int (*frame_callback_fun)(struct camera_control_block *ccb, struct frame_type *f);

typedef int (*device_run_fun)(struct camera_control_block *ccb,
                               frame_callback_fun cbk
                             );
typedef int (*device_shutdown_fun)();
typedef int (*device_suspend_fun)();
typedef int (*device_wakeup_fun)();


typedef struct {
  device_run_fun device_run;
  device_shutdown_fun device_shutdown;
  device_suspend_fun device_suspend;
  device_wakeup_fun device_wakeup;
} dev_interface;



int ltr_int_cal_run(struct camera_control_block *ccb, frame_callback_fun cbk);

/* call to shutdown the inited device
 * typically called once at close
 * turns all leds off
 * must call init to restart again after a shutdown
 * a return value < 0 indicates error */
int ltr_int_cal_shutdown();

/* suspend the currently inited camera device.
 * All leds on the camera will be deactivated
 * call cal_wakeup to un-suspend
 * the frame queue will be flushed
 * a return value < 0 indicates error */
int ltr_int_cal_suspend();

/* unsuspend the currently suspended (and inited)
 * camera device.
 * a return value < 0 indicates error */
int ltr_int_cal_wakeup();

linuxtrack_state_type ltr_int_cal_get_state();
void ltr_int_cal_set_state(linuxtrack_state_type new_state);
void ltr_int_change_state(enum ltr_request_t new_req);
enum ltr_request_t ltr_int_get_state_request();
void ltr_int_set_status_change_cbk(ltr_status_update_callback_t status_change_cbk, void *param);
bool ltr_int_got_new_request();

/* frees the memory allocated to the given frame.
 * For every frame populated, with cal_populate_frame,
 * this must be called when finished with the frame to
 * prevent memory leaks.
 * a return value < 0 indicates error */
void ltr_int_frame_free(struct camera_control_block *ccb,
                struct frame_type *f);

/* primarily for debug, will print a string
 * represtentation of the given frame to stdout */
void ltr_int_frame_print(struct frame_type f);

#ifdef __cplusplus
}
#endif

#endif
