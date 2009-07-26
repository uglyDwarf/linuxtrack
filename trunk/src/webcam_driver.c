#define _GNU_SOURCE
#include <features.h>
#include <stdio.h>
#undef _GNU_SOURCE

#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <stdbool.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include "webcam_driver.h"
#include "utils.h"
#include "pref.h"
#include "image_process.h"

#define NUM_OF_BUFFERS 8
typedef struct {
        void *start;
        size_t length;
} mmap_buffer;

mmap_buffer *buffers = NULL;


typedef struct{
  int fd;
  int expecting_blobs;
  bool is_diag;
  int buffers;
  int w;
  int h;
  unsigned char *bw_frame;
  unsigned int threshold;
  int min_blob_pixels;
  int max_blob_pixels;
} webcam_info;

webcam_info wc_info;

/*************/
/* interface */
/*************/

dev_interface webcam_interface = {
  .device_init = webcam_init,
  .device_shutdown = webcam_shutdown,
  .device_suspend = webcam_suspend,
  .device_change_operating_mode = webcam_change_operating_mode,
  .device_wakeup = webcam_wakeup,
  .device_set_good_indication = NULL,
  .device_get_frame = webcam_get_frame,
};

int is_webcam(char *fname, char *webcam_id)
{
  int fd = open(fname, O_RDWR);
  if(fd == -1){
    log_message("Can't open file '%s'!\n", fname);
    return -1;
  }
  
  struct v4l2_capability capability;

  //Query device capabilities
  int ioctl_res = ioctl(fd, VIDIOC_QUERYCAP, &capability);
  if(ioctl_res == 0){
    __u32 cap = capability.capabilities;
    log_message("  Found V4L2 webcam: '%s' at %s\n", 
      		capability.card, fname);
    //Look for capabilities we need
    if((cap & V4L2_CAP_VIDEO_CAPTURE) && 
      (cap & V4L2_CAP_STREAMING)){
      //we like this device ;-)
      if(strncasecmp((char *)capability.card, webcam_id, strlen(webcam_id)) == 0){
        //this is the device we are looking for!
        return fd;
      }
    }else{
      log_message("  Found V4L2 webcam but it doesn't support streaming:-(\n");
    }
  }
  return -1;
}


int search_for_webcam(char *webcam_id)
{
  if(webcam_id == NULL){
    log_message("Please spacify webcam Id!\n");
    return -1;
  }
  DIR *dev = opendir("/dev");
  int wfd = -1;
  if(dev == NULL){
    log_message("Can't open /dev for reading!\n");
    return -1;
  }
  struct dirent *de;
  while((de = readdir(dev)) != NULL){
    if(strncmp("video", de->d_name, 5) == 0){
      char *fname;
      asprintf(&fname, "/dev/%s", de->d_name);
      if((wfd = is_webcam(fname, webcam_id)) != -1){
        log_message("Found webcam '%s' (%s)\n", de->d_name, fname);
        free(fname);
        break;
      }
      free(fname);
    }
  }
  closedir(dev);
  return wfd;
}

bool read_pref_format(struct v4l2_format *fmt)
{
  memset(fmt, 0, sizeof(struct v4l2_format));
  
  char *res = get_key("Webcam", "Resolution");
  if(res == NULL){
    log_message("No resolution specified!\n");
    return false;
  }
  char *pix = get_key("Webcam", "Pixel-format");
  if(pix == NULL){
    log_message("No pixel format specified!\n");
    return false;
  }
  
  int x, y;
  if(sscanf(res, "%d x %d", &x, &y)!= 2){
    log_message("I don't understand resolution specified as '%s'!\n", res);
    return false;
  } 
  
  fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt->fmt.pix.width = x;
  fmt->fmt.pix.height = y;
  fmt->fmt.pix.pixelformat = *(__u32*)pix;
  fmt->fmt.pix.field = V4L2_FIELD_ANY;
  
  return true;
}


bool set_capture_format(struct camera_control_block *ccb)
{
  struct v4l2_format fmt;
  if(read_pref_format(&fmt) != true){
    return false;
  }
  
  if(0 != ioctl(wc_info.fd, VIDIOC_S_FMT, &fmt)){
    switch(errno){
      case EBUSY:
        log_message("Can't switch formats right now!\n");
        break;
      case EINVAL:
        log_message("Using wrong data to switch formats!\n");
        break;
    }
    return false;
  }
  ccb->pixel_width = wc_info.w = fmt.fmt.pix.width;
  ccb->pixel_height = wc_info.h = fmt.fmt.pix.height;
  wc_info.bw_frame = (unsigned char *)my_malloc(wc_info.w * wc_info.h);
  log_message("Switch of the format successfull!\n");
  return true;
}

bool set_stream_params()
{
  char *fps = get_key("Webcam", "Fps");
  if(fps == NULL){
    log_message("No framerate specified!\n");
    return false;
  }
  int num, den;
  if(sscanf(fps, "%d/%d", &num, &den)!= 2){
    log_message("I don't understand fps specified as '%s'!\n", fps);
    return false;
  } 

  struct v4l2_streamparm sp;
  memset(&sp, 0, sizeof(sp));
  sp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  //Switched num and den - to get time instead of fps;
  sp.parm.capture.timeperframe.numerator = den;
  sp.parm.capture.timeperframe.denominator = num;

  if(-1 == ioctl(wc_info.fd, VIDIOC_S_PARM, &sp)){
    log_message("Stream parameters setup failed! (%s)\n", strerror(errno));
    return false;
  }
  return true;
}


/*
 * Sends request to driver for mmap-able buffers
 * Uses constant NUM_OF_BUFFERS
 *
 * Returns number of buffers granted
 */
int request_streaming_buffers()
{
  struct v4l2_requestbuffers reqb;
  memset(&reqb, 0, sizeof(reqb));
  reqb.count = NUM_OF_BUFFERS;
  reqb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqb.memory = V4L2_MEMORY_MMAP;
  
  //request buffers from driver
  if(0 != ioctl(wc_info.fd, VIDIOC_REQBUFS, &reqb)){
    log_message("Couldn't get streaming buffers! (%s)\n", strerror(errno));
    return 0;
  }
  if(reqb.count < NUM_OF_BUFFERS){
    log_message("Got fewer buffers than expected! (%d instead of %d)\n",
                reqb.count, NUM_OF_BUFFERS);
  }else{
    log_message("Got %d buffers...\n", reqb.count);
  }
  return reqb.count;
}

/*
 * Prepares buffers for streaming (mmaps them and allocs
 * space for output BW frames)
 * 
 * Returns TRUE on success, FALSE otherwise
 */
bool setup_streaming_buffers()
{
  log_message("Setting up buffers for streaming...\n");
  wc_info.buffers = request_streaming_buffers();
  if(0 == wc_info.buffers){
      log_message("Request for buffers failed...\n");
      return false;
  }
  //alloc memory for array of buffers
  buffers = my_malloc(wc_info.buffers * sizeof(mmap_buffer));
  memset(buffers, 0, sizeof(mmap_buffer) * wc_info.buffers);
  
  //initialize buffer structures... 
  int cntr;
  for(cntr = 0; cntr < wc_info.buffers; ++cntr){
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = cntr;

    if(0 != ioctl(wc_info.fd, VIDIOC_QUERYBUF, &buf)){
      log_message("Request for buffer failed...\n");
      return false;
    }
    
    buffers[cntr].length = buf.length;
    buffers[cntr].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
      MAP_SHARED, wc_info.fd, buf.m.offset);
    if(MAP_FAILED == buffers[cntr].start){
      log_message("Mmap failed...\n");
      return false;
    }
  }
  log_message("Mmap setup successfull!\n");
  return true;
}


/*
 * Unmaps and frees streaming buffers...
 *  
 * Returns TRUE on success, FALSE otherwise
 */
bool release_buffers()
{
  if(NULL == buffers){
    log_message("Trying to release already released buffers...\n");
    return false;
  }
  int cntr;
  for(cntr = 0; cntr < wc_info.buffers; ++cntr){
    if(-1 == munmap(buffers[cntr].start, buffers[cntr].length)){
      log_message("Munmap failed!\n");
    }
  }
  free(buffers);
  buffers = NULL;
  log_message("Buffers unmapped!\n");
  return true;
}

bool read_img_processing_prefs()
{
  char *thres = get_key("Webcam", "Threshold");
  if(thres == NULL){
    log_message("No threshold specified!\n");
    return false;
  }
  char *max = get_key("Webcam", "Max-blob");
  if(max == NULL){
    log_message("No maximal pixel count for blob specified!\n");
    return false;
  }
  char *min = get_key("Webcam", "Min-blob");
  if(min == NULL){
    log_message("No minimal pixel count for blob specified!\n");
    return false;
  }
  wc_info.threshold = (unsigned int)atoi(thres);
  wc_info.min_blob_pixels = atoi(min);
  wc_info.max_blob_pixels = atoi(max);
  return true;
}

/*
 * I'm going to actively ignore resolution and I'll set it from prefs...
 */
int webcam_init(struct camera_control_block *ccb)
{
  assert(ccb != NULL);
  assert(ccb->device.category == webcam);
  assert(ccb->device.device_id != NULL);
  int fd = search_for_webcam(ccb->device.device_id);
  if(fd == -1){
    return -1;
  }
  wc_info.fd = fd;
  webcam_change_operating_mode(ccb, ccb->mode);
  
  if(set_capture_format(ccb) != true){
    log_message("Couldn't set capture format!\n");
    close(fd);
    return -1;
  }
  if(set_stream_params() != true){
    log_message("Couldn't set stream parameters!\n");
    close(fd);
    return -1;
  }
  if(setup_streaming_buffers() != true){
    log_message("Couldn't initialize mmap!\n");
    close(fd);
    return -1;
  }
  if(read_img_processing_prefs() != true){
    log_message("Couldn't initialize mmap!\n");
    close(fd);
    return -1;
  }
  ccb->state = suspended;
  if(webcam_wakeup(ccb) != 0){
    log_message("Couldn't start streaming!\n");
    close(fd);
    return -1;
  }
  return 0;
}

int webcam_shutdown(struct camera_control_block *ccb)
{
  if(ccb->state == active){
      webcam_suspend(ccb);

  }
  release_buffers();
  free(wc_info.bw_frame);
  ccb->state = pre_init;
  close(wc_info.fd);
  return 0;
}

int webcam_wakeup(struct camera_control_block *ccb)
{
  log_message("Queuing buffers...\n");
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  int cntr;
  for(cntr = 0; cntr < wc_info.buffers; ++cntr){
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = cntr;

    if(0 != ioctl(wc_info.fd, VIDIOC_QBUF, &buf)){
      log_message("Queuing of buffer failed...\n");
      return -1;
    }
  }
  log_message("Buffers queued, starting to stream!\n");
  
  if(-1 == ioctl(wc_info.fd, VIDIOC_STREAMON, &type)){
    log_message("Start of streaming failed!\n");
    return -1;
  }
  ccb->state = active;
  return 0;
}

int webcam_suspend(struct camera_control_block *ccb)
{
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(-1 == ioctl(wc_info.fd, VIDIOC_STREAMOFF, &type)){
    log_message("Problem stopping streaming!\n");
    return -1;
  }
  log_message("Streaming stopped!\n");
  ccb->state = suspended;
  return 0;
}

int webcam_change_operating_mode(struct camera_control_block *ccb, 
                             enum cal_operating_mode newmode)
{
  ccb->mode = newmode;
  switch(ccb->mode){
    case diagnostic:
      wc_info.is_diag = true;
      wc_info.expecting_blobs = -1;
      break;
    case operational_1dot:
      wc_info.is_diag = false;
      wc_info.expecting_blobs = 1;
      break;
    case operational_3dot:
      wc_info.is_diag = false;
      wc_info.expecting_blobs = 3;
      break;
    default:
      assert(0);
      break;
  } 
  return 0;   
}


int webcam_get_frame(struct camera_control_block *ccb, struct frame_type *f)
{
  f->bloblist.num_blobs = wc_info.expecting_blobs;

  struct v4l2_buffer buf;
  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  if(-1 == ioctl(wc_info.fd, VIDIOC_DQBUF, &buf)){
    switch(errno){
      case EAGAIN:
        return -1; //FIXME!!! Should call ioctl again...
      case EIO:
            /* Could ignore EIO, see spec. */
            /* fall through */
      default:
        log_message("Problem dequeing buffer! (%s)\n", strerror(errno));
        return -1;
    }
  }
  //There seems to be some kind of race condition - the above ioctl
  //blocks forever and by this log it seems to go away...
  log_message("");
  assert(buf.index < wc_info.buffers);
  
  //Convert YUYV format to bw image
  //FIXME!!! - this would work only with YUYV!
  unsigned char *source_buf = (buffers[buf.index]).start;
  unsigned char *dest_buf = wc_info.bw_frame;
  int cntr, cntr1;
//  int pts = 0;
  
  for(cntr = cntr1 = 0; cntr < buf.bytesused; cntr += 2, cntr1++){
    if(source_buf[cntr] > wc_info.threshold){
      dest_buf[cntr1] = source_buf[cntr];
//      ++pts;
    }else{
      dest_buf[cntr1] = 0;
    }
  }
  
//  printf("%d points found!\n", pts);

  if(wc_info.is_diag == true){
    f->bitmap = (char *)my_malloc(wc_info.w * wc_info.h);
    memcpy(f->bitmap, dest_buf, wc_info.w * wc_info.h);
  }
  
  if(-1 == ioctl(wc_info.fd, VIDIOC_QBUF, &buf)){
    log_message("Error queuing buffer!\n");
  }
  //log_message("Queued buffer %d\n", buf.index);
  
  if(search_for_blobs(wc_info.bw_frame, wc_info.w, wc_info.h, &(f->bloblist),
                   wc_info.min_blob_pixels, wc_info.max_blob_pixels) != true){
    return -1;
  }
  
  return 0;
}
