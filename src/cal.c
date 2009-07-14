/*************************************************************
 ****************** CAMERA ABSTRACTION LAYER *****************
 *************************************************************/
#include "cal.h"
#include "tir4.h"
#include "webcam.h"

/*********************/
/* private Constants */
/*********************/
#define WEBCAM_FILENAME "/dev/video0"
#define WEBCAM_BITSPERPIXEL "/dev/video0"

/**************************/
/* private Static members */
/**************************/
static struct camera_control_block *ctrl_blk;

/************************/
/* function definitions */
/************************/
bool cal_is_device_present(enum cal_device)
{
  switch (cal_device) {
  case tir4_camera:
    return tir4_is_device_present();
    break;
  case webcam:
    return webcam_is_device_present();
    break;
  case wiimote:
    return wiimote_is_device_present();
    break;
  }
}

int cal_init(struct camera_control_block *ccb)
{
  ctrl_blk = *ccb;
  switch (ctrl_blk.device) {
  case tir4_camera:
    return tir4_init(ctrl_blk.mode);
    break;
  case webcam:
    return webcam_init(ctrl_blk.mode,
                       WEBCAM_FILENAME,
                       ctrl_blk.pixel_width,
                       ctrl_blk.pixel_height,
                       WEBCAM_BITSPERPIXEL,
                       );
    break;
  case wiimote:
    return wiimote_init(ctrl_blk.mode);
    break;
  }
}

int cal_shutdown(void)
{
  switch (ctrl_blk.device) {
  case tir4_camera:
    return tir4_shutdown();
    break;
  case webcam:
    return webcam_shutdown();
    break;
  case wiimote:
    return wiimote_shutdown();
    break;
}

int cal_suspend(void)
{
  switch (ctrl_blk.device) {
  case tir4_camera:
    return tir4_suspend();
    break;
  case webcam:
    return webcam_suspend();
    break;
  case wiimote:
    return wiimote_suspend();
    break;
}

void cal_changeoperating_mode(struct camera_control_block ccb)
{
  ctrl_blk.mode = ccb.mode;
}

int cal_wakeup(void)
{
  switch (ctrl_blk.device) {
  case tir4_camera:
    return tir4_wakeup();
    break;
  case webcam:
    return webcam_wakeup();
    break;
  case wiimote:
    return wiimote_wakeup();
    break;
}

void cal_set_good_indication(bool arg)
{
  switch (ctrl_blk.device) {
  case tir4_camera:
    return tir4_set_good_indication();
    break;
  case webcam:
    /* do nothing */
    break;
  case wiimote:
    /* do nothing (?) */
    break;
}

int cal_do_read_and_process(void)
{
  switch (ctrl_blk.device) {
  case tir4_camera:
    return tir4_do_read_and_process();
    break;
  case webcam:
    return webcam_do_read_and_process();
    break;
  case wiimote:
    return wiimote_do_read_and_process();
    break;
}

bool cal_is_frame_available(void)
{
  switch (ctrl_blk.device) {
  case tir4_camera:
    return tir4_is_frame_available();
    break;
  case webcam:
    return webcam_is_frame_available();
    break;
  case wiimote:
    return wiimote_is_frame_available();
    break;
}

int cal_populate_frame(struct frame_type *f)
{
  switch (ctrl_blk.device) {
  case tir4_camera:
    return tir4_populate_frame(struct frame_type *f);
    break;
  case webcam:
    return webcam_populate_frame(struct frame_type *f);
    break;
  case wiimote:
    return wiimote_populate_frame(struct frame_type *f);
    break;
}

void cal_flush_frames(unsigned int n)
{
  switch (ctrl_blk.device) {
  case tir4_camera:
    return tir4_flush_frames(unsigned int n);
    break;
  case webcam:
    return webcam_flush_frames(unsigned int n);
    break;
  case wiimote:
    return wiimote_flush_frames(unsigned int n);
    break;
}

void frame_print(struct frame_type f)
{

}

void frame_free(struct frame_type *f)
{
  int i;
  for(i=0;i<f->bloblist->num_blobs;i++)
    free(f->bloblist->blobs[i]);
  
}


