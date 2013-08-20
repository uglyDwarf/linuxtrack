
//#include <stdarg.h>
#include <pthread.h>
#include "pref_global.h"
#include "utils.h" 
#include "pref.h"
#include "cal.h"
#include "tracking.h"
#include "ltlib_int.h"

static pthread_t cal_thread;
static ltr_new_frame_callback_t ltr_new_frame_cbk = NULL;
static void *ltr_new_frame_cbk_param = NULL;

static int frame_callback(struct camera_control_block *ccb, struct frame_type *frame)
{
  (void)ccb;
  ltr_int_update_pose(frame);
  if(ltr_new_frame_cbk != NULL){
    ltr_new_frame_cbk(frame, ltr_new_frame_cbk_param);
  }
  return 0;
}

static struct camera_control_block ccb;

static void *cal_thread_fun(void *param)
{
  (void)param;
  if(ltr_int_get_device(&ccb)){
    ccb.diag = false;
    ltr_int_cal_run(&ccb, frame_callback);
    free(ccb.device.device_id);
  }else{
    ltr_int_log_message("Couldn't get the device!\n");
  }
  //pthread_detach(pthread_self());
  return NULL;
}

int ltr_int_init(void)
{
  if(!ltr_int_read_prefs(NULL, false)){
    ltr_int_log_message("Couldn't load preferences!\n");
    return -1;
  }
  if(!ltr_int_init_tracking()){
    ltr_int_log_message("Couldn't initialize tracking!\n");
    return -1;
  }
  pthread_create(&cal_thread, NULL, cal_thread_fun, NULL);
  return 0;
}

int ltr_int_get_camera_update(pose_t *pose)
{
  return ltr_int_tracking_get_pose(pose);
}

void ltr_int_register_cbk(ltr_new_frame_callback_t new_frame_cbk, void *param1,
                          ltr_status_update_callback_t status_change_cbk, void *param2)
{
  ltr_new_frame_cbk = new_frame_cbk;
  ltr_new_frame_cbk_param = param1;
  ltr_int_set_status_change_cbk(status_change_cbk, param2);
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
  ltr_int_log_message("Shutting down tracking...\n");
  int res = ltr_int_cal_shutdown();
  pthread_join(cal_thread, NULL);
  return res;
}

void ltr_int_recenter(void)
{
  ltr_int_recenter_tracking();
}

ltr_state_type ltr_int_get_tracking_state(void)
{
  return ltr_int_cal_get_state();
}

