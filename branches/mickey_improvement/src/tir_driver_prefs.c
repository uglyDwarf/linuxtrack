#include <string.h>
#include <stdlib.h>
#include "tir_driver_prefs.h"
#include "pref.h"
#include "pref_global.h"

static int max_blob = 0;
static int min_blob = 0;
static int status_bright = 0;
static int ir_bright = 0;
static int threshold = 0;
static bool status = false;
static bool grayscale = false;

static char max_blob_key[] = "Max-blob";
static char min_blob_key[] = "Min-blob";
static char status_bright_key[] = "Status-led-brightness";
static char ir_bright_key[] = "Ir-led-brightness";
static char threshold_key[] = "Threshold";
static char status_key[] = "Status-signals";
static char grayscale_key[] = "Grayscale";

bool ltr_int_tir_init_prefs()
{
  const char *dev = ltr_int_get_device_section();
  if(dev == NULL){
    return false;
  }
  
  if(!ltr_int_get_key_int(dev, max_blob_key, &max_blob)){
    max_blob = 1024;
  }
  if(!ltr_int_get_key_int(dev, min_blob_key, &min_blob)){
    min_blob = 4;
  }
  if(!ltr_int_get_key_int(dev, status_bright_key, &status_bright)){
    status_bright = 0;
  }
  if(!ltr_int_get_key_int(dev, ir_bright_key, &ir_bright)){
    ir_bright = 7;
  }
  if(!ltr_int_get_key_int(dev, threshold_key, &threshold)){
    threshold = 140;
  }
  char *tmp = ltr_int_get_key(dev, status_key);
  if(tmp != NULL){
    status = (strcasecmp(tmp, "On") == 0) ? true : false;
    free(tmp);
  }else{
    status = true;
  }
  tmp = ltr_int_get_key(dev, grayscale_key);
  if(tmp != NULL){
    grayscale = (strcasecmp(tmp, "Yes") == 0) ? true : false;
  }else{
    grayscale = false;
  }
  return true;
}

int ltr_int_tir_get_max_blob()
{
  return max_blob;
}

bool ltr_int_tir_set_max_blob(int val)
{
  if(val < 0){
    val = 0;
  }
  max_blob = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), max_blob_key, val);
}

int ltr_int_tir_get_min_blob()
{
  return min_blob;
}

bool ltr_int_tir_set_min_blob(int val)
{
  if(val < 0){
    val = 0;
  }
  min_blob = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), min_blob_key, val);
}

int ltr_int_tir_get_status_brightness()
{
  return status_bright;
}

bool ltr_int_tir_set_status_brightness(int val)
{
  if(val < 0){
    val = 0;
  }
  if(val > 3){
    val = 3;
  }
  status_bright = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), status_bright_key, val);
}

int ltr_int_tir_get_ir_brightness()
{
  return ir_bright;
}

bool ltr_int_tir_set_ir_brightness(int val)
{
  if(val < 5){
    val = 5;
  }
  if(val > 7){
    val = 7;
  }
  ir_bright = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), ir_bright_key, val);
}

int ltr_int_tir_get_threshold()
{
  return threshold;
}

bool ltr_int_tir_set_threshold(int val)
{
  if(val < 30){
    val = 30;
  }
  if(val > 253){
    val = 253;
  }
  threshold = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), threshold_key, val);
}

bool ltr_int_tir_get_status_indication()
{
  return status;
}

bool ltr_int_tir_set_status_indication(bool ind)
{
  char on_val[] = "On";
  char off_val[] = "Off";
  char *res = ind ? on_val : off_val;
  status = ind;
  return ltr_int_change_key(ltr_int_get_device_section(), status_key, res);
}

bool ltr_int_tir_set_use_grayscale(bool gs)
{
  char on_val[] = "Yes";
  char off_val[] = "No";
  char *res = gs ? on_val : off_val;
  grayscale = gs;
  return ltr_int_change_key(ltr_int_get_device_section(), grayscale_key, res);
}

bool ltr_int_tir_get_use_grayscale()
{
  return grayscale;
}
