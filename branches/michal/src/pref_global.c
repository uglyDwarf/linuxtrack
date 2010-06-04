#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "pref.h"
#include "pref_int.h"
#include "pref_global.h"
#include "utils.h"
#include "axis.h"

#include "pathconfig.h"

static bool model_changed_flag = true;
static bool model_type_changed = true;
static pref_id dev_section = NULL;
static pref_id model_section = NULL;
static pref_id pref_model_type = NULL;
static pref_id cff = NULL;

void ltr_int_close_prefs()
{
  if(dev_section != NULL){
    ltr_int_close_pref(&dev_section);
    dev_section = NULL;
  }
  if(model_section != NULL){
    ltr_int_close_pref(&model_section);
    model_section = NULL;
  }
  if(pref_model_type != NULL){
    ltr_int_close_pref(&pref_model_type);
    pref_model_type = NULL;
  }
  if(cff != NULL){
    ltr_int_close_pref(&cff);
    cff = NULL;
  }
  ltr_int_close_axes(NULL);
}

static void pref_change_callback(void *param)
{
  assert(param != NULL);
  *(bool*)param = true;
}

static void model_changed_callback(void *param)
{
  assert(param != NULL);
  *(bool*)param = true;
}

char *ltr_int_get_device_section()
{
  if(dev_section == NULL){
    if(!ltr_int_open_pref("Global", "Input", &dev_section)){
      ltr_int_log_message("Entry 'Input' missing in 'Global' section!\n");
      return NULL;
    }
  }
  return ltr_int_get_str(dev_section);
}

static bool model_section_changed = true;

static char *ltr_int_get_model_section()
{
  static char *name;
  if(model_section == NULL){
    if(!ltr_int_open_pref_w_callback("Global", "Model", &model_section,
      model_changed_callback, (void*)&model_section_changed)){
      ltr_int_log_message("Entry 'Model' missing in 'Global' section!\n");
      return NULL;
    }
    model_section_changed = true;
  }
  
//  if(model_section_changed){
    name = ltr_int_get_str(model_section);
//  }
  return name;
}

bool ltr_int_is_model_active()
{
  char *section = ltr_int_get_model_section();
  pref_id active;
  if(!ltr_int_open_pref(section, "Active", &active)){
    ltr_int_log_message("Unspecified if model is active, assuming it is not...\n");
    return false;
  }
  bool res = (strcasecmp(ltr_int_get_str(active), "yes") == 0) ? true : false;
  ltr_int_close_pref(&active);
  return res;
}

bool ltr_int_get_device(struct camera_control_block *ccb)
{
  bool dev_ok = false;
  char *dev_section = ltr_int_get_device_section();
  if(dev_section == NULL){
    return false;
  }
  const char *dev_type = ltr_int_get_key(dev_section, "Capture-device");
  if (dev_type == NULL) {
    dev_ok = false;
  } else {
    if(strcasecmp(dev_type, "Tir") == 0){
      ltr_int_log_message("Device Type: Track IR\n");
      ccb->device.category = tir;
      dev_ok = true;
    }
    if(strcasecmp(dev_type, "Tir4") == 0){
      ltr_int_log_message("Device Type: Track IR 4\n");
      ccb->device.category = tir4_camera;
      dev_ok = true;
    }
    if(strcasecmp(dev_type, "Webcam") == 0){
      ltr_int_log_message("Device Type: Webcam\n");
      ccb->device.category = webcam;
      dev_ok = true;
    }
    if(strcasecmp(dev_type, "Wiimote") == 0){
      ltr_int_log_message("Device Type: Wiimote\n");
      ccb->device.category = wiimote;
      dev_ok = true;
    }
    if(dev_ok == false){
      ltr_int_log_message("Wrong device type found: '%s'\n", dev_type);
      ltr_int_log_message(" Valid options are: 'Tir4', 'Tir', 'Tir_openusb', 'Webcam', 'Wiimote'.\n");
    }
  }
  
  const char *dev_id = ltr_int_get_key(dev_section, "Capture-device-id");
  if (dev_id == NULL) {
    dev_ok = false;
  }else{
    ccb->device.device_id = ltr_int_my_strdup(dev_id);//!!! Benign memory leak!!!
  }
  
  return dev_ok;
}

bool ltr_int_get_coord(char *coord_id, float *f)
{
  char *model_section = ltr_int_get_model_section();
  if(model_section == NULL){
    return false;
  }
  const char *str = ltr_int_get_key(model_section, coord_id);
  if(str == NULL){
    ltr_int_log_message("Cannot find key %s in section %s!\n", coord_id, model_section);
    return false;
  }
  *f = atof(str);
  return true;
}

typedef enum {X, Y, Z, H_Y, H_Z} cap_index;
static pref_id cap_prefs[] = {NULL, NULL, NULL, NULL, NULL};

static bool setup_cap(reflector_model_type *rm, char *model_section)
{
  static char *ids[] = {"Cap-X", "Cap-Y", "Cap-Z", "Head-Y", "Head-Z"};
  ltr_int_log_message("Setting up Cap\n");
  cap_index i;
  for(i = X; i<= H_Z; ++i){
    if(!ltr_int_open_pref_w_callback(model_section, ids[i], &(cap_prefs[i]), 
      pref_change_callback, (void *)&model_changed_flag)){
      ltr_int_log_message("Couldn't setup Cap!\n");
      return false;
    }
  }
  
  float x = ltr_int_get_flt(cap_prefs[X]);
  float y = ltr_int_get_flt(cap_prefs[Y]);
  float z = ltr_int_get_flt(cap_prefs[Z]);
  float hy = ltr_int_get_flt(cap_prefs[H_Y]);
  float hz = ltr_int_get_flt(cap_prefs[H_Z]);
  
  rm->p1[0] = -x/2;
  rm->p1[1] = -y;
  rm->p1[2] = -z;
  rm->p2[0] = +x/2;
  rm->p2[1] = -y;
  rm->p2[2] = -z;
  rm->hc[0] = 0.0;
  rm->hc[1] = -hy;
  rm->hc[2] = hz;
  rm->type = CAP;
  return true;
}

typedef enum {Y1, Y2, Z1, Z2, HX, HY, HZ} clip_index;
static pref_id clip_prefs[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static bool setup_clip(reflector_model_type *rm, char *model_section)
{
  ltr_int_log_message("Setting up Clip...\n");
  static char *ids[] = {"Clip-Y1", "Clip-Y2", "Clip-Z1", "Clip-Z2", 
  			"Head-X", "Head-Y", "Head-Z"};
  
  clip_index i;
  for(i = Y1; i<= HZ; ++i){
    if(!ltr_int_open_pref_w_callback(model_section, ids[i], &(clip_prefs[i]),
      pref_change_callback, (void *)&model_changed_flag)){
      ltr_int_log_message("Couldn't setup Clip!\n");
      return false;
    }
  }
  
  float y1 = ltr_int_get_flt(clip_prefs[Y1]);
  float y2 = ltr_int_get_flt(clip_prefs[Y2]);
  float z1 = ltr_int_get_flt(clip_prefs[Z1]);
  float z2 = ltr_int_get_flt(clip_prefs[Z2]);
  float hx = ltr_int_get_flt(clip_prefs[HX]);
  float hy = ltr_int_get_flt(clip_prefs[HY]);
  float hz = ltr_int_get_flt(clip_prefs[HZ]);

  /*
  y1 is vertical dist of upper and middle point
  y2 is vertical dist of upper and lower point
  z1 is horizontal dist of upper and middle point
  z2 is horizontal dist of uper and lower point
  hx,hy,hz are head center coords with upper point as origin
  */ 
  
  rm->p1[0] = 0;
  rm->p1[1] = -y1;
  rm->p1[2] = z1;
  rm->p2[0] = 0;
  rm->p2[1] = -y2;
  rm->p2[2] = -z2;
  rm->hc[0] = hx;
  rm->hc[1] = hy;
  rm->hc[2] = hz;
  rm->type = CLIP;
  return true;
}

static void close_models()
{
  ltr_int_log_message("Closing models\n");
  clip_index i;
  for(i = Y1; i<= HZ; ++i){
    if(clip_prefs[i] != NULL){
      ltr_int_close_pref(&(clip_prefs[i]));
      clip_prefs[i] = NULL;
    }
  }
  cap_index j;
  for(j = X; j<= H_Z; ++j){
    if(cap_prefs[j] != NULL){
      ltr_int_close_pref(&(cap_prefs[j]));
      cap_prefs[j] = NULL;
    }
  }
  ltr_int_log_message("Done.\n");
}

bool ltr_int_model_changed()
{
  return model_section_changed || model_type_changed || model_changed_flag;
}

bool ltr_int_get_model_setup(reflector_model_type *rm)
{
  assert(rm != NULL);
  static char *model_section = NULL;
  if(model_section_changed){
    model_section_changed = false;
    if(pref_model_type != NULL){
      ltr_int_close_pref(&pref_model_type);
      pref_model_type = NULL;
    }
    model_section = ltr_int_get_model_section();
    if(model_section == NULL){
      ltr_int_log_message("Can't find model section!\n");
      return false;
    }
  }
  assert(model_section != NULL);
  if(pref_model_type == NULL){
    if(!ltr_int_open_pref_w_callback(model_section, "Model-type", &pref_model_type,
      pref_change_callback, (void *)&model_type_changed)){
      ltr_int_log_message("Couldn't find Model-type!\n");
      return false;
    }
    model_type_changed = true;
    model_changed_flag = true;
  }
  if(model_type_changed){
    model_type_changed = false;
    close_models();
  }
  static bool res = false;
  if(model_changed_flag){
    model_changed_flag = false;
    char *model_type = ltr_int_get_str(pref_model_type);
    assert(model_type != NULL);

    if(strcasecmp(model_type, "Cap") == 0){
      res = setup_cap(rm, model_section);
    }else if(strcasecmp(model_type, "Clip") == 0){
      res = setup_clip(rm, model_section);
    }else if(strcasecmp(model_type, "SinglePoint") == 0){
      rm->type = SINGLE;
      res = true;
    }else{
      ltr_int_log_message("Unknown modeltype specified in section %s\n", model_section);
      res = false;
    }
  }
  return res;
}


bool ltr_int_get_filter_factor(float *ff)
{
  if(cff == NULL){
    if(ltr_int_open_pref(ltr_int_get_custom_section_name(), "Filter-factor", &cff) != true){
      ltr_int_log_message("Can't read scale factor prefs!\n");
      return false;
    } 
  }
  *ff = ltr_int_get_flt(cff);
  return true;
}



