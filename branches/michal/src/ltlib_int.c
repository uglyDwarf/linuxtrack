
#include <stdarg.h>
#include "pref_global.h"
#include "utils.h" 
#include "pref_int.h"
#include "cal.h"
#include "tracking.h"

static pthread_t cal_thread;

static int frame_callback(struct camera_control_block *ccb, struct frame_type *frame)
{
  (void)ccb;
  ltr_int_update_pose(frame);
  return 0;
}

static struct camera_control_block ccb;

static void *cal_thread_fun(void *param)
{
  (void)param;
  if(ltr_int_get_device(&ccb)){
    ccb.diag = false;
    ltr_int_cal_run(&ccb, frame_callback);
    ltr_int_close_prefs();
  }
  return NULL;
}

int ltr_int_init(char *cust_section)
{
  if(!ltr_int_read_prefs(NULL, false)){
    ltr_int_log_message("Couldn't load preferences!\n");
    return -1;
  }
  ltr_int_set_custom_section(cust_section);
  if(!ltr_int_init_tracking()){
    ltr_int_log_message("Couldn't initialize trcking!\n");
    return -1;
  }
  pthread_create(&cal_thread, NULL, cal_thread_fun, NULL);
  return 0;
}

int ltr_int_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz)
{
  return ltr_int_tracking_get_camera(heading, pitch,roll, tx, ty, tz);
}

int ltr_int_suspend(void)
{
  return ltr_int_cal_suspend();
}

int ltr_int_wakeup(void)
{
  return ltr_int_cal_wakeup();
}

int ltr_int_shutdown(void)
{
  return ltr_int_cal_shutdown();
}

void ltr_int_recenter(void)
{
  ltr_int_recenter_tracking();
}

ltr_state_type ltr_int_get_tracking_state(void)
{
  return ltr_int_cal_get_state();
}

