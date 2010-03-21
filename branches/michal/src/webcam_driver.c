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
#include <sys/poll.h>
#include "webcam_driver.h"
#include "utils.h"
#include "pref.h"
#include "pref_global.h"
#include "image_process.h"
#include "pref_int.h"
#include "runloop.h"

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

tracker_interface trck_iface = {
  .tracker_init = webcam_init,
  .tracker_pause = webcam_suspend,
  .tracker_get_frame = webcam_get_frame,
  .tracker_resume = webcam_wakeup,
  .tracker_close = webcam_shutdown
};

/*************/
/* interface */
/*************/

int webcam_change_operating_mode(struct camera_control_block *ccb,
                             enum cal_operating_mode newmode);
int webcam_wakeup();
int webcam_suspend();



char *get_webcam_id(int fd)
{
  struct v4l2_capability capability;

  //Query device capabilities
  int ioctl_res = ioctl(fd, VIDIOC_QUERYCAP, &capability);
  if(ioctl_res == 0){
    __u32 cap = capability.capabilities;
    log_message("  Found V4L2 webcam: '%s'\n", 
      		capability.card);
    //Look for capabilities we need
    if((cap & V4L2_CAP_VIDEO_CAPTURE) && 
      (cap & V4L2_CAP_STREAMING)){
      return my_strdup((char *)capability.card);
    }else{
      log_message("  Found V4L2 webcam but it doesn't support streaming:-(\n");
    }
  }
  return NULL;
}

int is_our_webcam(char *fname, char *webcam_id)
{
  int fd = open(fname, O_RDWR | O_NONBLOCK);
  if(fd == -1){
    log_message("Can't open file '%s'!\n", fname);
    return -1;
  }
  
  char *current_id = get_webcam_id(fd);
  if((current_id != NULL) && strncasecmp(current_id, webcam_id, strlen(webcam_id)) == 0){
    //this is the device we are looking for!
    free(current_id);
    return fd;
  }else{
    free(current_id);
    close(fd);
  }
  return -1;
}

int enum_webcams(char **ids[])
{
  assert(ids != NULL);
  int counter = 1; //Already plus one!!!
  plist wc_list = create_list();
  char *id;
  DIR *dev = opendir("/dev");
  if(dev == NULL){
    log_message("Can't open /dev for reading!\n");
    return -1;
  }
  struct dirent *de;
  //Get list of all wabcams
  while((de = readdir(dev)) != NULL){
    if(strncmp("video", de->d_name, 5) == 0){
      char *fname;
      asprintf(&fname, "/dev/%s", de->d_name);
      
      int fd = open(fname, O_RDWR | O_NONBLOCK);
      if(fd == -1){
	log_message("Can't open file '%s'!\n", fname);
	return -1;
      }
      
      id = get_webcam_id(fd);
      if(id != NULL){
	++counter;
	add_element(wc_list, id); 
      }
      close(fd);
      free(fname);
    }
  }
  closedir(dev);
  //Convert list to array
  return list2string_list(wc_list, ids);
}


int search_for_webcam(char *webcam_id);

int enum_webcam_formats(char *id, webcam_formats *all_formats)
{
  int fd = search_for_webcam(id);
  if(fd < 0){
    return -1;
  }
  
  int fmt_cntr = 0;
  int sizes_cntr;
  int ival_cntr;
  int items = 0;
  struct v4l2_fmtdesc fmt = {0};
  struct v4l2_frmsizeenum frm = {0};
  struct v4l2_frmivalenum ival = {0};
  plist fmt_strings = create_list();
  plist formats = create_list();
  
  //Enumerate all available formats
  while(1){
    fmt.index = fmt_cntr;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd, VIDIOC_ENUM_FMT, &fmt) != 0){
      break;
    }
    ++fmt_cntr;
    add_element(fmt_strings, my_strdup((char *)fmt.description));
    sizes_cntr = 0;
    while(1){
      frm.index = sizes_cntr++;
      frm.pixel_format = fmt.pixelformat;
      if(ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frm) != 0){
	break;
      }
      if(frm.type == V4L2_FRMSIZE_TYPE_DISCRETE){
	ival_cntr = 0;
	while(1){
	  ival.index = ival_cntr++;
	  ival.pixel_format = fmt.pixelformat;
	  ival.width = frm.discrete.width;
	  ival.height = frm.discrete.height;
	  if(ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &ival) != 0){
	    break;
	  }
	  if(ival.type == V4L2_FRMIVAL_TYPE_DISCRETE){
	    webcam_format *new_fmt = (webcam_format*)my_malloc(sizeof(webcam_format));
	    new_fmt->i = fmt_cntr - 1;
	    new_fmt->fourcc = fmt.pixelformat;
	    new_fmt->w = frm.discrete.width;
	    new_fmt->h = frm.discrete.height;
	    new_fmt->fps_num = ival.discrete.numerator;
	    new_fmt->fps_den = ival.discrete.denominator;
	    add_element(formats, new_fmt);
	    ++items;
	  }
	}
      }
    }
  }
  char **strs = (char **)my_malloc((fmt_cntr + 1) * sizeof(char *));
  int cntr = 0;
  iterator i;
  char *desc;
  init_iterator(fmt_strings, &i);
  
  while((desc = (char *)get_next(&i)) != NULL){
    strs[cntr++] = desc;
  }
  strs[cntr] = NULL;
  all_formats->fmt_strings = strs;

  webcam_format *fmt_array = (webcam_format*)my_malloc(items * sizeof(webcam_format));
  init_iterator(formats, &i);
  webcam_format *wf;
  cntr = 0;
  while((wf = (webcam_format*)get_next(&i)) != NULL){
    fmt_array[cntr++] = *wf;
  }
  all_formats->formats = fmt_array;
  all_formats->entries = items;
  
  free_list(formats, true);
  free_list(fmt_strings, false);
  close(fd);
  return items;
}

int enum_webcam_formats_cleanup(webcam_formats *all_formats)
{
  int j = 0;
  while(all_formats->fmt_strings[j] != NULL){
    free(all_formats->fmt_strings[j++]);
  }
  free(all_formats->fmt_strings);
  all_formats->fmt_strings = NULL;
  free(all_formats->formats);
  all_formats->formats = NULL;
  return 0;
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
      if((wfd = is_our_webcam(fname, webcam_id)) != -1){
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
  
  char *dev_section = get_device_section();
  if(dev_section == NULL){
    return false;
  }
  char *res = get_key(dev_section, "Resolution");
  if(res == NULL){
    log_message("No resolution specified!\n");
    return false;
  }
  char *pix = get_key(dev_section, "Pixel-format");
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
  char *dev_section = get_device_section();
  if(dev_section == NULL){
    return false;
  }
  char *fps = get_key(dev_section, "Fps");
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
  char *dev_section = get_device_section();
  if(dev_section == NULL){
    return false;
  }
  char *thres = get_key(dev_section, "Threshold");
  if(thres == NULL){
    log_message("No threshold specified!\n");
    return false;
  }
  char *max = get_key(dev_section, "Max-blob");
  if(max == NULL){
    log_message("No maximal pixel count for blob specified!\n");
    return false;
  }
  char *min = get_key(dev_section, "Min-blob");
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
  prepare_for_processing(ccb->pixel_width, ccb->pixel_height);
  if(webcam_wakeup() != 0){
    log_message("Couldn't start streaming!\n");
    close(fd);
    return -1;
  }
  return 0;
}

int webcam_shutdown()
{
  log_message("Webcam shutting down!\n");
  release_buffers();
  free(wc_info.bw_frame);
  close(wc_info.fd);
  return 0;
}

int webcam_wakeup()
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
  return 0;
}

int webcam_suspend()
{
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(-1 == ioctl(wc_info.fd, VIDIOC_STREAMOFF, &type)){
    log_message("Problem stopping streaming!\n");
    return -1;
  }
  log_message("Streaming stopped!\n");
  return 0;
}

int webcam_change_operating_mode(struct camera_control_block *ccb, 
                             enum cal_operating_mode newmode)
{
  ccb->mode = newmode;
  switch(ccb->mode){
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
  f->width = wc_info.w;
  f->height = wc_info.h;
  struct v4l2_buffer buf;
  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  int res;
  struct pollfd pfd = {
    .fd = wc_info.fd,
    .events = POLLIN | POLLRDNORM,
    .revents = 0
  };
  
  while(1){
    res = poll(&pfd, 1, 500);
    if(res == 1){
      if(pfd.revents == (POLLIN | POLLRDNORM)){
        //we have data!
        break;
      }else{
        if((pfd.revents && POLLERR) != 0){
          log_message("Poll returned error (%s)!\n", strerror(errno));
	}else{
	  log_message("Poll returned unexpected event! (%X)\n",pfd.revents);
	}
      }
    }else if(res == -1){
      log_message("Poll returned error! (%s)", strerror(errno));
      return -1;
    }else if(res == 0){
      log_message("Poll timed out!\n");
    }else{
      log_message("Poll returned unexpected value %d!\n", res);
      return -1;
    }
  };
  
  while(-1 == ioctl(wc_info.fd, VIDIOC_DQBUF, &buf)){
    switch(errno){
      case EAGAIN:
        continue;
        break;
      default:
        log_message("Problem dequeing buffer! (%s)\n", strerror(errno));
        return -1;
    }
  }
  assert(buf.index < wc_info.buffers);
  
  //Convert YUYV format to bw image
  //FIXME!!! - this would work only with YUYV!
  unsigned char *source_buf = (buffers[buf.index]).start;
  unsigned char *dest_buf = (f->bitmap != NULL) ? f->bitmap : wc_info.bw_frame;
  int cntr, cntr1;
  //int pts = 0;
  
  for(cntr = cntr1 = 0; cntr < buf.bytesused; cntr += 2, cntr1++){
    if(source_buf[cntr] > wc_info.threshold){
      dest_buf[cntr1] = source_buf[cntr];
      //++pts;
    }else{
      dest_buf[cntr1] = 0;
    }
  }
  
  //log_message("%d points found!\n", pts);

  if(-1 == ioctl(wc_info.fd, VIDIOC_QBUF, &buf)){
    log_message("Error queuing buffer!\n");
  }
  //log_message("Queued buffer %d\n", buf.index);
  image img = {
    .bitmap = dest_buf,
    .w = wc_info.w,
    .h = wc_info.h,
    .ratio = 1.0f
  };
  to_stripes(&img);
  stripes_to_blobs(3, &(f->bloblist), wc_info.min_blob_pixels, 
		   wc_info.max_blob_pixels, &img);
  return 0;
}
