/*************************************************************
 ****************** CAMERA ABSTRACTION LAYER *****************
 *************************************************************/
#ifndef CAL__H
#define CAL__H
#include <stdbool.h>


#define CAL_DEVICE_OPEN_ERR -200
#define CAL_PROBABLE_PERMISSIONS_ERR -201
#define CAL_UNABLE_TO_FIND_DEVICE_ERR -202
#define CAL_CAPTURE_THREAD_CREATION_ERR -203
#define CAL_DISCONNECTED_ERR -204
#define CAL_UNKNOWN_ERR -205
#define CAL_INVALID_OPERATION_ERR -206

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
  /* array of blobs, they will come from the driver 
   * already sorted in area.  The driver will allocate
   * memory for these, and the caller must call 
   * frame_free(frame) to free these when finished */
  struct blob_type *blobs; 
};

struct frame_type {
  struct bloblist_type bloblist; 
  char *bitmap; /* 8bits per pixel, monochrome 0x00 or 0xff */
};

typedef enum cal_device_category_type {
  tir4_camera,
  webcam,
  wiimote
} cal_device_category_type;

enum cal_device_state_type {
  pre_init, /* before init called or after shutdown */
  active, /* after init and not suspended */
  suspended /* suspended, back to active on wakeup */
};

struct cal_device_type {
  enum cal_device_category_type category;
  char *device_id;
};

enum cal_operating_mode {
  /* operational_1dot reports frames with 1 or more blobs, and 
   * only the first blob is populated.  The bitmap field 
   * in the frames are NOT populated. */
  operational_1dot, 
  /* operational_3dot only reports frames with 3 or more blobs, 
   * and only the first 3 blobs are populated.  The bitmap field 
   * in the frames are NOT populated. */
  operational_3dot, 
  /* diagnostic reports all frames with 1 or more blobs. The 
   * bitmap field in the frames ARE populated. */
  diagnostic
};

struct camera_control_block {
  struct cal_device_type device;
  unsigned int pixel_width;
  unsigned int pixel_height;
  enum cal_operating_mode mode;
  enum cal_device_state_type state;
};

/* call to init an uninitialized camera device 
 * typically called once at setup
 * this function may block for up to 3 seconds
 * this function will use the control block's device for 
 * the device to use.  The x & y resolutions are used 
 * as a guideline for allocating a resolution, but init will 
 * populate the x & y resolutions with the actual resolution.
 * in the updated control block 
 * the operating mode will be set to the operating mode in the 
 * control block 
 * a return value < 0 indicates error */
int cal_init(struct camera_control_block *ccb);

/* call to shutdown the inited device 
 * typically called once at close
 * turns all leds off
 * must call init to restart again after a shutdown 
 * a return value < 0 indicates error */
int cal_shutdown(struct camera_control_block *ccb);

/* suspend the currently inited camera device. 
 * All leds on the camera will be deactivated
 * call cal_wakeup to un-suspend 
 * the frame queue will be flushed
 * a return value < 0 indicates error */
int cal_suspend(struct camera_control_block *ccb);

/* may only be called while suspended.  Used to change from 
 * operational mode mode to diagnostic mode based on the mode 
 * field in the control block argument (and vice versa)
 * ONLY the operating mode may change! */
void cal_change_operating_mode(struct camera_control_block *ccb,
                               enum cal_operating_mode newmode);

/* unsuspend the currently suspended (and inited) 
 * camera device. 
 * a return value < 0 indicates error */
int cal_wakeup(struct camera_control_block *ccb);

/* arg=True to tell the device to indicate the 
 * tracking is good.
 * arg=True to tell the device to indicate the 
 * tracking is missing.
 * in the TIR4 case, this will set the colored 
 * LED on the camera to either green (good) or 
 * red (bad). 
 * a return value < 0 indicates error */
int cal_set_good_indication(struct camera_control_block *ccb,
                            bool arg);


/* This will read the camera port, and process it into 
 * frames and pull a frame off the camera's internal 
 * frame queue, and use it to populate the frame 
 * argument.
 * To populate a frame, memory is allocated; be
 * sure to call frame_free when done with the frame!
 * a return value < 0 indicates error */
int cal_get_frame(struct camera_control_block *ccb,
                  struct frame_type *f);

/* Start a capture thread, this WILL BLOCK until the 
 * thread has actually started; this should be a very short 
 * period */
void cal_thread_start(struct camera_control_block *ccb);


/* Request termination of capture thread
 * note that the thread may not actually stop if its 
 * hung up looking for a frame.  It will stop when the 
 * it gets a chance though.  use the cal_thread_is_stopped()
 * below to poll until its really stopped, if needed */ 
void cal_thread_stop(void);

/* returns true if the capture thread is actually stopped,
 * technically should not be needed */
bool cal_thread_is_stopped(void);


/* Access function meant to be used from outside.
 * return_frame is assumed to be a freed frame, 
 * (no bitmap or blobs allocated). 
 * be sure to free the frame returned when done with it!
 * returns false if unable to get a new frame */
int cal_thread_get_frame(struct frame_type *return_frame, 
                         bool *return_frame_valid);

/* frees the memory allocated to the given frame.  
 * For every frame populated, with cal_populate_frame,
 * this must be called when finished with the frame to 
 * prevent memory leaks.
 * a return value < 0 indicates error */
void frame_free(struct camera_control_block *ccb,
                struct frame_type *f);

/* primarily for debug, will print a string 
 * represtentation of the given frame to stdout */ 
void frame_print(struct frame_type f);

void bloblist_print(struct bloblist_type bl);

void blob_print(struct blob_type b);

#endif
