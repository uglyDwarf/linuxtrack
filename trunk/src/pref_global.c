
#include <string.h>
#include <stdlib.h>
#include "pref.h"
#include "pref_global.h"

bool get_device(enum cal_device_category_type *dev_type_enum)
{
  bool dev_ok = false;
  char *dev_type = get_key("Global", "Capture-device");
  if(strcmp(dev_type, "Tir4") == 0){
    log_message("Device Type: Track IR 4\n");
    *dev_type_enum = tir4_camera;
    dev_ok = true;
  }
  if(strcmp(dev_type, "Webcam") == 0){
    log_message("Device Type: Webcam\n");
    *dev_type_enum = webcam;
    dev_ok = true;
  }
  if(strcmp(dev_type, "Wiimote") == 0){
    log_message("Device Type: Wiimote\n");
    *dev_type_enum = wiimote;
    dev_ok = true;
  }
  if(dev_ok == false){
    log_message("Wrong device type found: '%s'\n", dev_type);
    log_message(" Valid options are: 'Tir4', 'Webcam', 'Wiimote'.\n");
  }
  return dev_ok;
}

bool get_coord(char *coord_id, float *f)
{
  char *str = get_key("Global", coord_id);
  if(str == NULL){
    log_message("Cannot find key %s in section global!\n", coord_id);
    return false;
  }
  *f = atof(str);
  return true;
}

bool setup_cap(reflector_model_type *rm)
{
  log_message("Setting up Cap...\n");
  float x,y,z,hy,hz;
  if(get_coord("Cap-X", &x) && get_coord("Cap-Y", &y) &&
     get_coord("Cap-Z", &z) && get_coord("Head-Y", &hy) &&
     get_coord("Head-Z", &hz) != true){
    log_message("Can't read-in Cap setup!\n");
    return false;
  }
  
  rm->p1[0] = -x/2;
  rm->p1[1] = -y;
  rm->p1[2] = -z;
  rm->p2[0] = -x/2;
  rm->p2[1] = -y;
  rm->p2[2] = -z;
  rm->hc[0] = 0.0;
  rm->hc[1] = -hy;
  rm->hc[2] = hz;
  return true;
}

bool setup_clip(reflector_model_type *rm)
{
  log_message("Setting up Clip... Not implemented yet:-(\n");
  
}



bool get_pose_setup(reflector_model_type *rm)
{
  char *model_type = get_key("Global", "Model-type");
  if(strcmp(model_type, "Cap") == 0){
    return setup_cap(rm);
  }
  if(strcmp(model_type, "Clip") == 0){
    return setup_clip(rm);
  }
  return false;
}

