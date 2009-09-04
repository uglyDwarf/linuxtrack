#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "pref_global.h"
#include "ltlib.h"
#include "utils.h" 
#include "math_utils.h"
#include "pref_int.h"
#include <math.h>
#include <assert.h>

/**************************/
/* private Static members */
/**************************/
static struct camera_control_block ccb;
static float filterfactor=1.0;

static struct lt_scalefactors scales = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
static struct bloblist_type filtered_bloblist;
static struct blob_type filtered_blobs[3];
static bool first_frame_read = false;

/*******************************/
/* private function prototypes */
/*******************************/
float expfilt(float x, 
              float y_minus_1,
              float filtfactor);

void expfilt_vec(float x[3], 
              float y_minus_1[3],
              float filtfactor,
              float res[3]);

float nonlinfilt(float x, 
              float y_minus_1,
              float filtfactor);

void nonlinfilt_vec(float x[3], 
              float y_minus_1[3],
              float filtfactor,
              float res[3]);

float clamp_angle(float angle);

/************************/
/* function definitions */
/************************/

bool check_pose()
{
  struct reflector_model_type rm;
  bool changed;
  if(get_pose_setup(&rm, &changed) == false){
    log_message("Can't get pose setup!\n");
    return false;
  }
  if(changed){
    pose_init(rm);
    if (rm.type == CAP) {
      ccb.enable_IR_illuminator_LEDS = true;
    }
    else if (rm.type == CLIP) {
      ccb.enable_IR_illuminator_LEDS = false;
    }
    else {
      log_message("Unknown Model-type!\n");
      return false;
    }
  }
  return true;
}

int lt_init(struct lt_configuration_type config, char *cust_section)
{
  
  set_custom_section(cust_section);
  
  if(get_filter_factor(&filterfactor) != true){
    return -1;
  }
  if(get_scale_factors(&scales) != true){
    return -1;
  }

  if(get_device(&ccb) == false){
    log_message("Can't get device category!\n");
    return -1;
  }
  if(check_pose() == false){
    log_message("Can't get pose setup!\n");
    return -1;
  }

  ccb.mode = operational_3dot;
  if(cal_init(&ccb)!= 0){
    return -1;
  }
  cal_set_good_indication(&ccb, true);
  cal_thread_start(&ccb);

  filtered_bloblist.num_blobs = 3;
  filtered_bloblist.blobs = filtered_blobs;
  first_frame_read = false;
  return 0;
}

//Purpose of this procedure is to modify translations to work more
//intuitively - if I rotate my head right and move it to the left,
//it should move view left, not back
void rotate_translations(float *heading, float *pitch, float *roll, 
                         float *tx, float *ty, float *tz)
{
  float tm[3][3];
  float k = 180.0 / M_PI;
  float p = *pitch / k;
  float y = *heading / k;
  float r = *roll / k;
  euler_to_matrix(p, y, r, tm);
  float tr[3] = {*tx, *ty, *tz};
  float res[3];
  transpose_in_place(tm);
  matrix_times_vec(tm, tr, res);
  *tx = res[0];
  *ty = res[1];
  *tz = res[2];
}

int lt_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz)
{
  float raw_angles[3];
  float raw_translations[3];
  static float filtered_angles[3] = {0.0f, 0.0f, 0.0f};
  static float filtered_translations[3] = {0.0f, 0.0f, 0.0f};
  struct transform t;
  struct frame_type frame;
  bool frame_valid;
  int retval;
  retval = cal_thread_get_frame(&frame, 
                                &frame_valid);
  if (retval < 0) {
    return retval; 
  }
  if (frame_valid) {
    check_pose();
    get_filter_factor(&filterfactor);
    if(frame.bloblist.num_blobs != 3){
      return -1;
    }
    if(is_finite(frame.bloblist.blobs[0].x) && is_finite(frame.bloblist.blobs[0].y) &&
       is_finite(frame.bloblist.blobs[1].x) && is_finite(frame.bloblist.blobs[1].y) &&
       is_finite(frame.bloblist.blobs[2].x) && is_finite(frame.bloblist.blobs[2].y)){
    }else{
      return -1;
    }
    pose_sort_blobs(frame.bloblist);
    int i;
    for(i=0;i<3;i++) {
      if (first_frame_read) {
        filtered_bloblist.blobs[i].x = nonlinfilt(frame.bloblist.blobs[i].x,
                                               filtered_bloblist.blobs[i].x,
                                               filterfactor);
        filtered_bloblist.blobs[i].y = nonlinfilt(frame.bloblist.blobs[i].y,
                                               filtered_bloblist.blobs[i].y,
                                               filterfactor);
      }
      else {
        filtered_bloblist.blobs[i].x = frame.bloblist.blobs[i].x;
        filtered_bloblist.blobs[i].y = frame.bloblist.blobs[i].y;
      }
    }
    if (!first_frame_read) {
      first_frame_read = true;
    }
/*     printf("*** RAW blobs ***\n"); */
/*     bloblist_print(frame.bloblist); */
/*     printf("*** filtered blobs ***\n"); */
/*     bloblist_print(filtered_bloblist); */
        
    pose_process_blobs(filtered_bloblist, &t);
/*     transform_print(t); */
    pose_compute_camera_update(t,
                               &raw_angles[0], //heading
                               &raw_angles[1], //pitch
                               &raw_angles[2], //roll
                               &raw_translations[0], //tx
                               &raw_translations[1], //ty
                               &raw_translations[2]);//tz
    frame_free(&ccb, &frame);
    
    if(is_finite(raw_angles[0]) && is_finite(raw_angles[1]) && 
      is_finite(raw_angles[2]) && is_finite(raw_translations[0]) && 
      is_finite(raw_translations[1]) && is_finite(raw_translations[2])){
	//intentionaly left empty
    }else{
      return -1;
    }

    nonlinfilt_vec(raw_angles, filtered_angles, filterfactor, filtered_angles);
    nonlinfilt_vec(raw_translations, filtered_translations, filterfactor, 
            filtered_translations);
    
  }  
  get_scale_factors(&scales);
  *heading = clamp_angle(scales.yaw_sf * filtered_angles[0]);
  *pitch = clamp_angle(scales.pitch_sf * filtered_angles[1]);
  *roll = clamp_angle(scales.roll_sf * filtered_angles[2]);
  *tx = scales.tx_sf * filtered_translations[0];
  *ty = scales.ty_sf * filtered_translations[1];
  *tz = scales.tz_sf * filtered_translations[2];
  
  rotate_translations(heading, pitch, roll, tx, ty, tz);
  
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
    first_frame_read = false;
    cal_set_good_indication(&ccb, true);
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

float expfilt(float x, 
              float y_minus_1,
              float filterfactor) 
{
  float y;
  
  y = y_minus_1*(1.0-filterfactor) + filterfactor*x;

  return y;
}

void expfilt_vec(float x[3], 
              float y_minus_1[3],
              float filterfactor,
              float res[3]) 
{
  res[0] = expfilt(x[0], y_minus_1[0], filterfactor);
  res[1] = expfilt(x[1], y_minus_1[1], filterfactor);
  res[2] = expfilt(x[2], y_minus_1[2], filterfactor);
}


float nonlinfilt(float x, 
              float y_minus_1,
              float filterfactor) 
{
  float y;
  float delta = x - y_minus_1;
  y = y_minus_1 + delta * (fabsf(delta)/(fabsf(delta) + filterfactor));

  return y;
}

void nonlinfilt_vec(float x[3], 
              float y_minus_1[3],
              float filterfactor,
              float res[3]) 
{
  res[0] = nonlinfilt(x[0], y_minus_1[0], filterfactor);
  res[1] = nonlinfilt(x[1], y_minus_1[1], filterfactor);
  res[2] = nonlinfilt(x[2], y_minus_1[2], filterfactor);
}


float clamp_angle(float angle)
{
  if(angle<-180.0){
    return -180.0;
  }else if(angle>180.0){
    return 180.0;
  }else{
    return angle;
  }
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
