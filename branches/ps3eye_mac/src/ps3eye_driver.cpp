#include <ps3eye.h>
#include "wc_driver_prefs.h"
#include "image_process.h"
#include "cal.h"

extern "C" {
int ltr_int_tracker_init(struct camera_control_block *ccb);
int ltr_int_tracker_close();
int ltr_int_tracker_pause();
int ltr_int_tracker_resume();
int ltr_int_tracker_get_frame(struct camera_control_block *ccb,
                   struct frame_type *f, bool *frame_acquired);
}

static ps3eye::PS3EYECam::PS3EYERef cam;

int ltr_int_tracker_init(struct camera_control_block *ccb)
{
  (void) ccb;
  std::vector<ps3eye::PS3EYECam::PS3EYERef> devices(ps3eye::PS3EYECam::getDevices());
  if(devices.size()){
    cam = devices[0];
    if(!cam->init(320, 240, 30)){
      ltr_int_log_message("Can't initialize PS3Eye camera.\n");
      return -1;
    }
    ltr_int_prepare_for_processing(320, 240);//!!!
    //ltr_int_prepare_for_processing(ccb->pixel_width, ccb->pixel_height);
    cam->start();
  }else{
    ltr_int_log_message("Can't find any PS3Eye camera.\n");
    return -1;
  }
  return 0;
}

int ltr_int_tracker_close()
{
  cam->stop();
  ltr_int_cleanup_after_processing();
  return 0;
}

int ltr_int_tracker_pause()
{
  cam->stop();
  return 0;
}

int ltr_int_tracker_resume()
{
  cam->start();
  return 0;
}

static unsigned int threshold = 128;

static void get_bw_image(const unsigned char *source_buf, unsigned char *dest_buf, unsigned int bytes_used)
{
  unsigned int cntr, cntr1;
  threshold = ltr_int_wc_get_threshold();
  for(cntr = cntr1 = 0; cntr < bytes_used; cntr += 2, ++cntr1){
    if(source_buf[cntr] > threshold){
      dest_buf[cntr1] = source_buf[cntr];
    }else{
      dest_buf[cntr1] = 0;
    }
  }
}

static unsigned char *frame = NULL;

static int  current_exposure = 120;
static int  current_gain = 20;
static int  current_brightness = 20;
static int  current_contrast = 37;
static int  current_sharpness = 0;
static bool current_agc = false;
static bool current_awb = false;


static void check_prefs()
{
  if(current_exposure != ltr_int_wc_get_exposure()){
    current_exposure = ltr_int_wc_get_exposure();
    cam->setExposure(current_exposure);
  }
  if(current_gain != ltr_int_wc_get_gain()){
    current_gain = ltr_int_wc_get_gain();
    cam->setGain(current_gain);
  }
  if(current_brightness != ltr_int_wc_get_brightness()){
    current_brightness = ltr_int_wc_get_brightness();
    cam->setBrightness(current_brightness);
  }
  if(current_contrast != ltr_int_wc_get_contrast()){
    current_contrast = ltr_int_wc_get_contrast();
    cam->setContrast(current_contrast);
  }
  if(current_sharpness != ltr_int_wc_get_sharpness()){
    current_sharpness = ltr_int_wc_get_sharpness();
    cam->setSharpness(current_sharpness);
  }
  if(current_agc != ltr_int_wc_get_agc()){
    current_agc = ltr_int_wc_get_agc();
    cam->setAutogain(current_agc);
  }
  if(current_awb != ltr_int_wc_get_awb()){
    current_awb = ltr_int_wc_get_awb();
    cam->setAutoWhiteBalance(current_awb);
  }
}

int ltr_int_tracker_get_frame(struct camera_control_block *ccb,
                   struct frame_type *f, bool *frame_acquired)
{
  (void) ccb;
  *frame_acquired = false;
  if(!ps3eye::PS3EYECam::updateDevices()){
    ltr_int_log_message("Problem updating device.\n");
    return -1;
  }
  f->width = cam->getWidth();
  f->height = cam->getHeight();
  if(cam->isNewFrame()){
    check_prefs();
    if(frame == NULL){
      frame = (unsigned char *)malloc(f->width * f->height);
    }
    const unsigned char *source_buf = (const unsigned char *)cam->getLastFramePointer();

    FILE *fr = fopen("/tmp/frame.bin", "w");
    if(fr){
      fwrite(source_buf, f->height * cam->getRowBytes(), 1, fr);
      fclose(fr);
    }

    unsigned char *dest_buf = (f->bitmap != NULL) ? f->bitmap : frame;
    get_bw_image(source_buf, dest_buf, cam->getRowBytes() * f->height);

    image_t img;
    img.bitmap = dest_buf;
    img.w = f->width;
    img.h = f->height;
    img.ratio = 1.0f;

#ifndef OPENCV
    ltr_int_to_stripes(&img);
    ltr_int_stripes_to_blobs(MAX_BLOBS, &(f->bloblist), ltr_int_wc_get_min_blob(),
                    ltr_int_wc_get_max_blob(), &img);
#else
    ltr_int_face_detect(&img, &(f->bloblist));
#endif
    *frame_acquired = true;
  }
  return 0;
}

