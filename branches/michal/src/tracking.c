#include "tracking.h"
#include "math_utils.h"
#include "pose.h"
#include "utils.h"
#include "pref_global.h"

static float filterfactor=1.0;

/**************************/
/* private Static members */
/**************************/

static struct lt_scalefactors scales = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
static struct bloblist_type filtered_bloblist;
static struct blob_type filtered_blobs[3];
static bool first_frame = true;
static bool recenter = false;
struct current_pose lt_current_pose;

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
  if(get_pose_setup(&rm) == false){
    log_message("Can't get pose setup!\n");
    return false;
  }
  pose_init(rm);
  return true;
}

bool init_tracking()
{
  if(get_filter_factor(&filterfactor) != true){
    return false;
  }
  if(get_scale_factors(&scales) != true){
    return false;
  }

  if(check_pose() == false){
    log_message("Can't get pose setup!\n");
    return false;
  }
  filtered_bloblist.num_blobs = 3;
  filtered_bloblist.blobs = filtered_blobs;
  first_frame = true;
  return true;
}

int recenter_tracking()
{
  recenter = true;
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

pthread_mutex_t pose_mutex = PTHREAD_MUTEX_INITIALIZER;

int update_pose(struct frame_type *frame)
{
  float raw_angles[3];
  float raw_translations[3];
  static float filtered_angles[3] = {0.0f, 0.0f, 0.0f};
  static float filtered_translations[3] = {0.0f, 0.0f, 0.0f};
  struct transform t;
  bool recentering = false;
  
  if(recenter == true){
    recenter = false;
    recentering = true;
    log_message("RECENTERING!\n");
  } 
  
  get_filter_factor(&filterfactor);
  if(frame->bloblist.num_blobs != 3){
    return -1;
  }
  if(is_finite(frame->bloblist.blobs[0].x) && is_finite(frame->bloblist.blobs[0].y) &&
      is_finite(frame->bloblist.blobs[1].x) && is_finite(frame->bloblist.blobs[1].y) &&
      is_finite(frame->bloblist.blobs[2].x) && is_finite(frame->bloblist.blobs[2].y)){
  }else{
    return -1;
  }
  pose_sort_blobs(frame->bloblist);

  int i;
  for(i=0;i<3;i++) {
    if (first_frame || recentering) {
      filtered_bloblist.blobs[i].x = frame->bloblist.blobs[i].x;
      filtered_bloblist.blobs[i].y = frame->bloblist.blobs[i].y;
    }else{
      filtered_bloblist.blobs[i].x = nonlinfilt(frame->bloblist.blobs[i].x,
                                              filtered_bloblist.blobs[i].x,
                                              filterfactor);
      filtered_bloblist.blobs[i].y = nonlinfilt(frame->bloblist.blobs[i].y,
                                              filtered_bloblist.blobs[i].y,
                                              filterfactor);
    }
  }
  if (first_frame) {
    first_frame = false;
  }
      
  log_message("[%f,%f], [%f, %f], [%f, %f]\n", filtered_bloblist.blobs[0].x, 
  filtered_bloblist.blobs[0].y,  filtered_bloblist.blobs[1].x,
  filtered_bloblist.blobs[1].y, filtered_bloblist.blobs[2].x,
  filtered_bloblist.blobs[2].y);
  pose_process_blobs(filtered_bloblist, &t, recentering);
/*     transform_print(t); */
  pose_compute_camera_update(t,
			      &raw_angles[0], //heading
			      &raw_angles[1], //pitch
			      &raw_angles[2], //roll
			      &raw_translations[0], //tx
			      &raw_translations[1], //ty
			      &raw_translations[2]);//tz
  
  if(is_finite(raw_angles[0]) && is_finite(raw_angles[1]) && 
    is_finite(raw_angles[2]) && is_finite(raw_translations[0]) && 
    is_finite(raw_translations[1]) && is_finite(raw_translations[2])){
      //intentionaly left empty
  }else{
    return -1;
  }
  
  if(first_frame || recentering){
    for(i = 0; i < 3; ++i){
      filtered_angles[i] = raw_angles[i];
      filtered_translations[i] = raw_translations[i];
    }
  }else{
    nonlinfilt_vec(raw_angles, filtered_angles, filterfactor, filtered_angles);
    nonlinfilt_vec(raw_translations, filtered_translations, filterfactor, 
          filtered_translations);
  }
  
  get_scale_factors(&scales);
  pthread_mutex_lock(&pose_mutex);
  
  lt_current_pose.heading = clamp_angle(scales.yaw_sf * filtered_angles[0]);
  lt_current_pose.pitch = clamp_angle(scales.pitch_sf * filtered_angles[1]);
  lt_current_pose.roll = clamp_angle(scales.roll_sf * filtered_angles[2]);
  lt_current_pose.tx = scales.tx_sf * filtered_translations[0];
  lt_current_pose.ty = scales.ty_sf * filtered_translations[1];
  lt_current_pose.tz = scales.tz_sf * filtered_translations[2];
  
  rotate_translations(&lt_current_pose.heading, &lt_current_pose.pitch, 
                      &lt_current_pose.roll, &lt_current_pose.tx,
		      &lt_current_pose.ty, &lt_current_pose.tz);
  pthread_mutex_unlock(&pose_mutex);
  return 0;
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

