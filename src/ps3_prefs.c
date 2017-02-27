#include "ps3_prefs.h"
#include "utils.h"
#include "pref.h"
#include "pref_global.h"


static char max_blob_key[] = "Max-blob";
static char min_blob_key[] = "Min-blob";
static char threshold_key[] = "Threshold";
static char id_key[] = "Capture-device-id";
static char autogain_key[] = "AutoGain";
static char awb_key[] = "AWB";
static char autoexposure_key[] = "AutoExposure";
static char gain_key[] = "Gain";
static char exposure_key[] = "Exposure";
static char brightness_key[] = "Brightness";
static char contrast_key[] = "Contrast";
static char sharpness_key[] = "Sharpness";
static char plfreq_key[] = "PowerLine-Frequency";
static char fps_key[] = "Fps";
static char mode_key[] = "Mode";
static char hue_key[] = "Hue";
static char hflip_key[] = "HFlip";
static char vflip_key[] = "VFlip";
static char saturation_key[] = "Saturation";

typedef struct{
  int def;
  int min;
  int max;
  int *val;
  const char *desc;
  const char *key;
  bool changed;
} t_control;


static int hue;
static int saturation;
static int autogain;
static int autowhitebalance;
static int autoexposure;
static int gain;
static int exposure;
static int brightness;
static int contrast;
static int sharpness;
static int hflip;
static int vflip;
static int plfreq;
static int fps;

static int threshold = 128;
static int min_blob = 4;
static int max_blob = 2500;

static int width, height;

static int ctrl_changed = 0;

static t_control controls[e_NUMCTRLS] = {
  {.def = 0,   .min = -90, .max = 90,  .val = &hue,              .desc = "Hue",
     .key = hue_key, .changed = false},
  {.def = 64,  .min = 0,   .max = 255, .val = &saturation,       .desc = "Saturation",
     .key = saturation_key, .changed = false},
  {.def = 0,   .min = 0,   .max = 1,   .val = &autogain,         .desc = "Autogain",
     .key = autogain_key, .changed = false},
  {.def = 0,   .min = 0,   .max = 1,   .val = &autowhitebalance, .desc = "Autowhitebalance",
     .key = awb_key, .changed = false},
  {.def = 0,   .min = 0,   .max = 3,   .val = &autoexposure,     .desc = "Autoexposure",
     .key = autoexposure_key, .changed = false},
  {.def = 20,  .min = 0,   .max = 63,  .val = &gain,             .desc = "Gain",
     .key = gain_key, .changed = false},
  {.def = 120, .min = 0,   .max = 255, .val = &exposure,         .desc = "Exposure",
     .key = exposure_key, .changed = false},
  {.def = 0,   .min = 0,   .max = 255, .val = &brightness,       .desc = "Brightness",
     .key = brightness_key, .changed = false},
  {.def = 32,  .min = 0,   .max = 255, .val = &contrast,         .desc = "Contrast",
     .key = contrast_key, .changed = false},
  {.def = 0,   .min = 0,   .max = 63,  .val = &sharpness,        .desc = "Sharpness",
     .key = sharpness_key, .changed = false},
  {.def = 0,   .min = 0,   .max = 1,   .val = &hflip,            .desc = "HFlip",
     .key = hflip_key, .changed = false},
  {.def = 0,   .min = 0,   .max = 1,   .val = &vflip,            .desc = "VFlip",
     .key = vflip_key, .changed = false},
  {.def = 0,   .min = 0,   .max = 3,   .val = &plfreq,           .desc = "Powerline freq.",
     .key = plfreq_key, .changed = false},
  {.def = 60,  .min = 15,  .max = 187, .val = &fps,              .desc = "FPS",
     .key = fps_key, .changed = false}
};

bool ltr_int_ps3_ctrl_changed(t_controls ctrl)
{
  if((ctrl < 0) || (ctrl >= e_NUMCTRLS)){
    ltr_int_log_message("Request to get changeflag of control No. %d (max is %d).\n", ctrl, e_NUMCTRLS - 1);
    return false;
  }
  return controls[ctrl].changed;
}

int ltr_int_ps3_controls_changed(void)
{
  return ctrl_changed;
}

int ltr_int_ps3_get_ctrl_val(t_controls ctrl)
{
  if((ctrl < 0) || (ctrl >= e_NUMCTRLS)){
    ltr_int_log_message("Request to get value of control No. %d (max is %d).\n", ctrl, e_NUMCTRLS - 1);
    return 0;
  }
  controls[ctrl].changed = false;
  return *(controls[ctrl].val);
}

bool ltr_int_ps3_set_ctrl_val(t_controls ctrl, int val)
{
  if((ctrl < 0) || (ctrl >= e_NUMCTRLS)){
    ltr_int_log_message("Request to set control No. %d (max is %d).\n", ctrl, e_NUMCTRLS - 1);
    return false;
  }
  t_control *c = &(controls[ctrl]);
  if((val < c->min) || (val > c->max)){
    ltr_int_log_message("Request to set %s to %d which is out of bounds <%d, %d>",
                          c->desc, val, c->min, c->max);
    return false;
  }
  *(c->val) = val;
  c->changed = true;
  ++ctrl_changed;
  return ltr_int_change_key_int(ltr_int_get_device_section(), c->key, val);
}

/*
ltr_int_ps3_get_hue()
ltr_int_ps3_get_saturation()
ltr_int_ps3_get_autogain()
ltr_int_ps3_get_autowhitebalance()
ltr_int_ps3_get_autoexposure()
ltr_int_ps3_get_gain()
ltr_int_ps3_get_exposure()
ltr_int_ps3_get_brightness()
ltr_int_ps3_get_contrast()
ltr_int_ps3_get_sharpness()
ltr_int_ps3_get_hflip()
ltr_int_ps3_get_vflip()
ltr_int_ps3_get_plfreq()
*/
/*
typedef struct {
  int w;
  int h;
  int fps;
} t_modes;

static t_modes modes[] = {
  {320, 240, 187}, // 0
  {320, 240, 150}, // 1
  {320, 240, 137}, // 2
  {320, 240, 120}, // 3
  {320, 240, 100}, // 4
  {320, 240,  75}, // 5
  {320, 240,  60}, // 6
  {320, 240,  50}, // 7
  {320, 240,  37}, // 8
  {320, 240,  30}, // 9
  {640, 480,  60}, //10
  {640, 480,  50}, //11
  {640, 480,  40}, //12
  {640, 480,  30}, //13
  {640, 480,  15}, //14
  { -1,  -1,  -1}
};
*/
static int mode = 0;

int ltr_int_ps3_get_mode(void)
{
  return mode;
}

static void ltr_int_mode_2_wh(int val)
{
  mode = val & 1;
  if(mode == 0){
    width = 320;
    height = 240;
  }else{
    width = 640;
    height = 480;
  }
}

bool ltr_int_ps3_set_mode(int val)
{
  ltr_int_mode_2_wh(val);
  return ltr_int_change_key_int(ltr_int_get_device_section(), mode_key, val);
}

bool ltr_int_ps3_init_prefs(void)
{
  int i;
  for(i = 0; i < e_NUMCTRLS; ++i){
    *(controls[i].val) = controls[i].def;
    controls[i].changed = true;
  }

  char *dev = ltr_int_get_device_section();
  if(dev == NULL){
    return false;
  }
  
  ltr_int_get_key_int(dev, max_blob_key, &max_blob);
  ltr_int_get_key_int(dev, min_blob_key, &min_blob);
  ltr_int_get_key_int(dev, threshold_key, &threshold);
  ltr_int_get_key_int(dev, autogain_key, &autogain);
  ltr_int_get_key_int(dev, awb_key, &autowhitebalance);
  ltr_int_get_key_int(dev, autoexposure_key, &autoexposure);
  ltr_int_get_key_int(dev, gain_key, &gain);
  ltr_int_get_key_int(dev, exposure_key, &exposure);
  ltr_int_get_key_int(dev, brightness_key, &brightness);
  ltr_int_get_key_int(dev, contrast_key, &contrast);
  ltr_int_get_key_int(dev, sharpness_key, &sharpness);
  ltr_int_get_key_int(dev, plfreq_key, &plfreq);
  ltr_int_get_key_int(dev, fps_key, &fps);
  ltr_int_get_key_int(dev, mode_key, &mode);
  
  ltr_int_mode_2_wh(mode);

  return true;
}

bool ltr_int_ps3_close_prefs(void)
{
  return true;
}

/*
bool ltr_int_ps3_set_resolution(int w, int h)
{
  width = w;
  height = h;
  return true;
}
*/

bool ltr_int_ps3_get_resolution(int *w, int *h)
{
  *w = width;
  *h = height;
  return true;
}


int ltr_int_ps3_get_threshold(void)
{
  return threshold;
}

bool ltr_int_ps3_set_threshold(int val)
{
  threshold = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), threshold_key, val);
}

int ltr_int_ps3_get_max_blob(void)
{
  return max_blob;
}

bool ltr_int_ps3_set_max_blob(int val)
{
  max_blob = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), max_blob_key, val);
}

int ltr_int_ps3_get_min_blob(void)
{
  return min_blob;
}

bool ltr_int_ps3_set_min_blob(int val)
{
  min_blob = val;
  return ltr_int_change_key_int(ltr_int_get_device_section(), min_blob_key, val);
}

