/*************************************************************
 ****************** CAMERA ABSTRACTION LAYER *****************
 *************************************************************/
#ifndef CAL__H
#define CAL__H

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
  struct bloblist_type  bloblist; 
  char **bitmap; /* 8bits per pixel, monochrome 0x00 or 0xff */
};

enum cal_device_type {
  tir4_camera,
  webcam, /* assumes /dev/video0 */
  wiimote
};

enum cal_operating_mode {
  /* operational only reports frames with 3 or more blobs, and 
   * only the first 3 blobs are populated.  The bitmap field 
   * in the frames are NOT populated. */
  operational, 
  /* diagnostic reports all frames with 1 or more blobs. The 
   * bitmap field in the frames ARE populated. */
  diagnostic
};

struct camera_control_block {
  enum cal_device_type device;
  unsigned int pixel_width;
  unsigned int pixel_height;
  enum cal_operating_mode mode;
};

/* given a device type, the cal will search for the 
 * device and return true if present */
bool cal_is_device_present(enum cal_device);

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
int cal_shutdown(void);

/* suspend the currently inited camera device. 
 * All leds on the camera will be deactivated
 * call cal_wakeup to un-suspend 
 * the frame queue will be flushed
 * a return value < 0 indicates error */
int cal_suspend(void);

/* may only be called while suspended.  Used to change from 
 * operational mode mode to diagnostic mode based on the mode 
 * field in the control block argument 
 * ONLY the operating mode may change! */
void cal_changeoperating_mode(struct camera_control_block ccb);

/* unsuspend the currently suspended (and inited) 
 * camera device. 
 * All leds on the camera will be deactivated
 * call cal_wakeup to un-suspend 
 * a return value < 0 indicates error */
int cal_wakeup(void);

/* arg=True to tell the device to indicate the 
 * tracking is good.
 * arg=True to tell the device to indicate the 
 * tracking is missing.
 * in the TIR4 case, this will set the colored 
 * LED on the camera to either green (good) or 
 * red (bad). 
 * a return value < 0 indicates error */
int cal_set_good_indication(bool arg);

/* read the camera port, and process it into frames
 * a return value < 0 indicates error */
int cal_do_read_and_process(void);

/* Will return true if there are any pending 
 * frames */
bool cal_is_frame_available(void);

/* this will pull a frame off the camera's internal 
 * frame queue, and use it to populate the frame 
 * argument.
 * the to populate a frame, memory is allocated; be
 * sure to call frame_free when done with the frame!
 * a return value < 0 indicates error */
int cal_populate_frame(struct frame_type *f);

/* directs the cal device to flush all but N frames in its
 * internal frame queue.  n=0 will competely flush the 
 * queue */
void cal_flush_frames(unsigned int n);

/* primarily for debug, will print a string 
 * represtentation of the given frame to stdout */ 
void frame_print(struct frame_type f);

/* frees the memory allocated to the given frame.  
 * For every frame populated, with cal_populate_frame,
 * this must be called when finished with the frame to 
 * prevent memory leaks.
 * a return value < 0 indicates error */
int frame_free(struct frame_type *f);

#endif
