#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tir_driver_prefs.h"
#include "pref.h"
#include "pref_global.h"
#include "utils.h"

static int max_blob = 0;
static int min_blob = 0;
static int threshold_val = 0;
static char *camera_id = NULL;
static char *pix_fmt = NULL;
static int res_x = 0;
static int res_y = 0;
static int fps_num = 0;
static int fps_den = 0;
static bool flip = false;
static char *cascade = NULL;
static float exp_filt = 0.1;
static int optim_level = 0;

static char max_blob_key[] = "Max-blob";
static char min_blob_key[] = "Min-blob";
static char threshold_key[] = "Threshold";
static char id_key[] = "Capture-device-id";
static char pix_fmt_key[] = "Pixel-format";
static char res_key[] = "Resolution";
static char fps_key[] = "Fps";
static char flip_key[] = "Upside-down";
static char cascade_key[] = "Cascade";
static char exp_filter_key[] = "Exp-filter-factor";
static char optim_key[] = "Optimization-level";

bool ltr_int_wc_init_prefs()
{
  char *dev = ltr_int_get_device_section();
  if(dev == NULL){
    return false;
  }
  
  if(!ltr_int_get_key_int(dev, max_blob_key, &max_blob)){
    max_blob = 1024;
  }
  if(!ltr_int_get_key_int(dev, min_blob_key, &min_blob)){
    min_blob = 4;
  }
  if(!ltr_int_get_key_int(dev, threshold_key, &threshold_val)){
    threshold_val = 140;
  }
  char *tmp = ltr_int_get_key(dev, id_key);
  if(tmp != NULL){
    camera_id = tmp;
  }
  tmp = ltr_int_get_key(dev, pix_fmt_key);
  if(tmp != NULL){
    pix_fmt = tmp;
  }
  tmp = ltr_int_get_key(dev, res_key);
  if(tmp != NULL){
    if(sscanf(tmp, "%d x %d", &res_x, &res_y)!= 2){
      res_x = res_y = -1;
    }
    free(tmp);
  }
  tmp = ltr_int_get_key(dev, fps_key);
  if(tmp != NULL){
    if(sscanf(tmp, "%d/%d", &fps_num, &fps_den)!= 2){
      fps_num = fps_den = -1;
    }
    free(tmp);
  }
  tmp = ltr_int_get_key(dev, flip_key);
  if(tmp != NULL){
    flip = (strcasecmp(tmp, "Yes") == 0) ? true : false;
    free(tmp);
  }else{
    flip = false;
  }
  
  tmp = ltr_int_get_key(dev, cascade_key);
  cascade = tmp;
  if(!ltr_int_get_key_flt(dev, exp_filter_key, &exp_filt)){
    exp_filt = 0.1;
  }
  if(!ltr_int_get_key_int(dev, optim_key, &optim_level)){
    optim_level= 0;
  }
  free(dev);
  return true;
}

void ltr_int_wc_close_prefs()
{
  free(camera_id);
  free(pix_fmt);
  free(cascade);
}

int ltr_int_wc_get_max_blob()
{
  return max_blob;
}

bool ltr_int_wc_set_max_blob(int val)
{
  if(val < 0){
    val = 0;
  }
  max_blob = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), max_blob_key, val);
}

int ltr_int_wc_get_min_blob()
{
  return min_blob;
}

bool ltr_int_wc_set_min_blob(int val)
{
  if(val < 0){
    val = 0;
  }
  min_blob = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), min_blob_key, val);
}

int ltr_int_wc_get_threshold()
{
  return threshold_val;
}

bool ltr_int_wc_set_threshold(int val)
{
  if(val < 30){
    val = 30;
  }
  if(val > 253){
    val = 253;
  }
  threshold_val = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), threshold_key, val);
}

const char *ltr_int_wc_get_id()
{
  return camera_id;
}



const char *ltr_int_wc_get_pixfmt()
{
  return pix_fmt;
}

bool ltr_int_wc_set_pixfmt(const char *fmt)
{
  assert(fmt != NULL);
  if(pix_fmt != NULL){
    free(pix_fmt);
  }
  pix_fmt = ltr_int_my_strdup(fmt);
  return ltr_int_change_key(ltr_int_get_device_section(), pix_fmt_key, fmt);
}

bool ltr_int_wc_get_resolution(int *x, int *y)
{
  if((res_x < 0) || (res_y < 0)){
    return false;
  }
  *x = res_x;
  *y = res_y;
  return true;
}

bool ltr_int_wc_set_resolution(int x, int y)
{
  char tmp[1024];
  if(snprintf(tmp, sizeof(tmp), "%d x %d", x, y) != 0){
    res_x = x;
    res_y = y;
    return ltr_int_change_key(ltr_int_get_device_section(), res_key, tmp);
  }else{
    return false;
  }
}

bool ltr_int_wc_get_fps(int *num, int *den)
{
  if((fps_num < 0) || (fps_den < 0)){
    return false;
  }
  *num = fps_num;
  *den = fps_den;
  return true;
}

bool ltr_int_wc_set_fps(int num, int den)
{
  char tmp[1024];
  if(snprintf(tmp, sizeof(tmp), "%d/%d", num, den) != 0){
    fps_num = num;
    fps_den = den;
    return ltr_int_change_key(ltr_int_get_device_section(), fps_key, tmp);
  }else{
    return false;
  }
}

bool ltr_int_wc_get_flip()
{
  return flip;
}

bool ltr_int_wc_set_flip(bool new_flip)
{
  char yes[] = "Yes";
  char no[] = "No";
  char *val = (new_flip) ? yes : no;
  flip = new_flip;
  return ltr_int_change_key(ltr_int_get_device_section(), flip_key, val);
}

const char *ltr_int_wc_get_cascade()
{
  return cascade;
}

bool ltr_int_wc_set_cascade(const char *new_cascade)
{
  if(cascade != NULL){
    free(cascade);
  }
  cascade = (new_cascade != NULL) ? ltr_int_my_strdup(new_cascade) : NULL;
  return ltr_int_change_key(ltr_int_get_device_section(), cascade_key, new_cascade);
}

float ltr_int_wc_get_eff()
{
  return exp_filt;
}

bool ltr_int_wc_set_eff(float new_eff)
{
  char tmp[1024];
  if(snprintf(tmp, sizeof(tmp), "%g", new_eff) != 0){
    exp_filt = new_eff;
    return ltr_int_change_key(ltr_int_get_device_section(), exp_filter_key, tmp);
  }else{
    return false;
  }
}

int ltr_int_wc_get_optim_level()
{
  return optim_level;
}

bool ltr_int_wc_set_optim_level(int opt)
{
  optim_level = opt;
  return ltr_int_change_key_int(ltr_int_get_device_section(), optim_key, opt);
}

