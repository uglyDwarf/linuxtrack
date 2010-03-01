
#include <stdarg.h>
#include "pref_global.h"
#include "utils.h" 
#include "pref_int.h"
#include "ltlib_client.h"
#include "cal.h"
#include "tracking.h"

int lt_int_init(char *cust_section)
{
  set_custom_section(cust_section);
  if(!read_prefs(NULL, false)){
    log_message("Couldn't load preferences!\n");
    return -1;
  }
  if(!init_tracking()){
    log_message("Couldn't initialize trcking!\n");
    return -1;
  }
  return lt_client_init();
}

int lt_int_get_camera_update(float *heading,
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

int lt_int_suspend(void)
{
  return lt_client_suspend();
}

int lt_int_wakeup(void)
{
  return lt_client_wakeup();
}

int lt_int_shutdown(void)
{
  return lt_client_close();
}

void lt_int_recenter(void)
{
  recenter_tracking();
}

bool lt_int_create_pref(char *key)
{
  return add_key(NULL, key, "");
}

bool lt_int_open_pref(char *key, pref_id *prf)
{
  return open_pref(NULL, key, prf);
}

float lt_int_get_flt(pref_id prf)
{
  return get_flt(prf);
}

int lt_int_get_int(pref_id prf)
{
  return get_int(prf);
}

char *lt_int_get_str(pref_id prf)
{
  return get_str(prf);
}

bool lt_int_set_flt(pref_id *prf, float f)
{
  return set_flt(prf, f);
}

bool lt_int_set_int(pref_id *prf, int i)
{
  return set_int(prf, i);
}

bool lt_int_set_str(pref_id *prf, char *str)
{
  return set_str(prf, str);
}

bool lt_int_save_prefs()
{
  return save_prefs();
}

bool lt_int_close_pref(pref_id *prf)
{
  return close_pref(prf);
}

void lt_int_log_message(const char *format, ...)
{
  va_list ap;
  va_start(ap,format);
  log_message(format, ap);
  va_end(ap);
}
