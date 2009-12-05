
//#include <stdio.h>
#include <stdarg.h>
//#include <string.h>
#include "pref_global.h"
//#include "ltlib.h"
#include "utils.h" 
#include "pref_int.h"
//#include <math.h>
//#include <assert.h>
#include <pthread.h>

#include "cal.h"
#include "tracking.h"

static struct camera_control_block ccb;


int lt_init(char *cust_section)
{
  
  set_custom_section(cust_section);
  
  if(get_device(&ccb) == false){
    log_message("Can't get device category!\n");
    return -1;
  }

  ccb.mode = operational_3dot;
  ccb.diag = false;
  if(cal_init(&ccb)!= 0){
    return -1;
  }
  if(cal_thread_start(&ccb) != 0){
    return -1;
  }
  if(!init_tracking(&ccb)){
    return -1;
  }
  return 0;
}

int lt_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz)
{
  pthread_mutex_lock(&pose_mutex);
  
  *heading = lt_current_pose.heading;
  *pitch = lt_current_pose.pitch;
  *roll = lt_current_pose.roll;
  *tx = lt_current_pose.tx;
  *ty = lt_current_pose.ty;
  *tz = lt_current_pose.tz;
  
  pthread_mutex_unlock(&pose_mutex);
  return 0;
}

int lt_suspend(void)
{
  if(ccb.state == suspended){
    return 0;
  }else{
    cal_thread_stop();
    return cal_suspend(&ccb);
  }
}

int lt_wakeup(void)
{
  if(ccb.state == active){
    return 0;
  }else{
    cal_thread_start(&ccb);
    return cal_wakeup(&ccb);
  }
}

int lt_shutdown(void)
{
  lt_wakeup();
  cal_thread_stop();
  cal_shutdown(&ccb);
  return 0;
}

void lt_recenter(void)
{
  pose_recenter();
}

bool lt_create_pref(char *key)
{
  return add_key(NULL, key, "");
}

bool lt_open_pref(char *key, pref_id *prf)
{
  return open_pref(NULL, key, prf);
}

float lt_get_flt(pref_id prf)
{
  return get_flt(prf);
}

int lt_get_int(pref_id prf)
{
  return get_int(prf);
}

char *lt_get_str(pref_id prf)
{
  return get_str(prf);
}

bool lt_set_flt(pref_id *prf, float f)
{
  return set_flt(prf, f);
}

bool lt_set_int(pref_id *prf, int i)
{
  return set_int(prf, i);
}

bool lt_set_str(pref_id *prf, char *str)
{
  return set_str(prf, str);
}

bool lt_save_prefs()
{
  return save_prefs();
}

bool lt_pref_changed(pref_id pref)
{
  return pref_changed(pref);
}

bool lt_close_pref(pref_id *prf)
{
  return close_pref(prf);
}

void lt_log_message(const char *format, ...)
{
  va_list ap;
  va_start(ap,format);
  log_message(format, ap);
  va_end(ap);
}
