#include "tracking.h"
#include "math_utils.h"
#include "pose.h"
#include "utils.h"
#include "pref_global.h"

static float filterfactor=1.0;

/**************************/
/* private Static members */
/**************************/

static struct bloblist_type filtered_bloblist;
static struct blob_type filtered_blobs[3];
static bool first_frame = true;
static bool recenter = false;
struct current_pose ltr_int_orig_pose;

/*******************************/
/* private function prototypes */
/*******************************/
/*static float expfilt(float x, 
              float y_minus_1,
              float filtfactor);

static void expfilt_vec(float x[3], 
              float y_minus_1[3],
              float filtfactor,
              float res[3]);
*/
static float nonlinfilt(float x, 
              float y_minus_1,
              float filtfactor);

static void nonlinfilt_vec(float x[3], 
              float y_minus_1[3],
              float filtfactor,
              float res[3]);

static float clamp_angle(float angle);

/************************/
/* function definitions */
/************************/

bool ltr_int_check_pose()
{
  struct reflector_model_type rm;
  if(ltr_int_model_changed()){
    if(ltr_int_get_model_setup(&rm) == false){
      ltr_int_log_message("Can't get pose setup!\n");
      return false;
    }
    ltr_int_log_message("Initializing model!\n");
    ltr_int_pose_init(rm);
  }
  return true;
}

static bool tracking_initialized = false;

bool ltr_int_init_tracking()
{
  if(ltr_int_get_filter_factor(&filterfactor) != true){
    return false;
  }

  if(ltr_int_check_pose() == false){
    ltr_int_log_message("Can't get pose setup!\n");
    return false;
  }
  filtered_bloblist.num_blobs = 3;
  filtered_bloblist.blobs = filtered_blobs;
  first_frame = true;
  ltr_int_log_message("Tracking initialized!\n");
  tracking_initialized = true;
  return true;
}

int ltr_int_recenter_tracking()
{
  recenter = true;
  return 0;
}

//Purpose of this procedure is to modify translations to work more
//intuitively - if I rotate my head right and move it to the left,
//it should move view left, not back
static void rotate_translations(float *heading, float *pitch, float *roll, 
                         float *tx, float *ty, float *tz)
{
  float tm[3][3];
  float k = 180.0 / M_PI;
  float p = *pitch / k;
  float y = *heading / k;
  float r = *roll / k;
  ltr_int_euler_to_matrix(p, y, r, tm);
  float tr[3] = {*tx, *ty, *tz};
  float res[3];
  ltr_int_transpose_in_place(tm);
  ltr_int_matrix_times_vec(tm, tr, res);
  *tx = res[0];
  *ty = res[1];
  *tz = res[2];
}

static pthread_mutex_t pose_mutex = PTHREAD_MUTEX_INITIALIZER;
static float raw_angles[3] = {0.0f, 0.0f, 0.0f};
static float raw_translations[3] = {0.0f, 0.0f, 0.0f};

static int update_pose_1pt(struct frame_type *frame)
{
  static float c_x = 0.0f;
  static float c_y = 0.0f;
  bool recentering = false;
  
  ltr_int_check_pose();
  
  
  if(ltr_int_is_finite(frame->bloblist.blobs[0].x) && ltr_int_is_finite(frame->bloblist.blobs[0].y)){
  }else{
    return -1;
  }

  if(recenter == true){
    recenter = false;
    recentering = true;
  } 
  
  if(recentering){
    c_x = frame->bloblist.blobs[0].x;
    c_y = frame->bloblist.blobs[0].y;
  }
  
  raw_angles[0] = frame->bloblist.blobs[0].x - c_x;
  raw_angles[1] = frame->bloblist.blobs[0].y - c_y;
  raw_angles[2] = 0.0f;
  raw_translations[0] = 0.0f;
  raw_translations[1] = 0.0f;
  raw_translations[2] = 0.0f;
  return 0;
}


static int update_pose_3pt(struct frame_type *frame)
{
  struct transform t;
  bool recentering = false;
  
  ltr_int_check_pose();

  if(frame->bloblist.num_blobs != 3){
    return -1;
  }
  if(ltr_int_is_finite(frame->bloblist.blobs[0].x) && ltr_int_is_finite(frame->bloblist.blobs[0].y) &&
      ltr_int_is_finite(frame->bloblist.blobs[1].x) && ltr_int_is_finite(frame->bloblist.blobs[1].y) &&
      ltr_int_is_finite(frame->bloblist.blobs[2].x) && ltr_int_is_finite(frame->bloblist.blobs[2].y)){
  }else{
    return -1;
  }
  if(recenter == true){
    recenter = false;
    recentering = true;
  } 
  
  ltr_int_pose_sort_blobs(frame->bloblist);
  ltr_int_pose_process_blobs(frame->bloblist, &t, recentering);
/*     transform_print(t); */
  return ltr_int_pose_compute_camera_update(t,
			      &raw_angles[0], //heading
			      &raw_angles[1], //pitch
			      &raw_angles[2], //roll
			      &raw_translations[0], //tx
			      &raw_translations[1], //ty
			      &raw_translations[2]);//tz
}

int ltr_int_update_pose(struct frame_type *frame)
{
  if(ltr_int_is_single_point()){
    return update_pose_1pt(frame);
  }else{
    return update_pose_3pt(frame);
  }
}

int ltr_int_tracking_get_camera(float *heading,
                      float *pitch,
                      float *roll,
                      float *tx,
                      float *ty,
                      float *tz)
{
  static float filtered_angles[3] = {0.0f, 0.0f, 0.0f};
  static float filtered_translations[3] = {0.0f, 0.0f, 0.0f};
  
  if(!tracking_initialized){
    ltr_int_init_tracking();
  }
  
  ltr_int_get_filter_factor(&filterfactor);
  nonlinfilt_vec(raw_angles, filtered_angles, filterfactor, filtered_angles);
  nonlinfilt_vec(raw_translations, filtered_translations, filterfactor, 
        filtered_translations);
  
  pthread_mutex_lock(&pose_mutex);
  ltr_int_orig_pose.heading = filtered_angles[0];
  ltr_int_orig_pose.pitch = filtered_angles[1];
  ltr_int_orig_pose.roll = filtered_angles[2];
  ltr_int_orig_pose.tx = filtered_translations[0];
  ltr_int_orig_pose.ty = filtered_translations[1];
  ltr_int_orig_pose.tz = filtered_translations[2];
  
  *heading = clamp_angle(ltr_int_val_on_axis(YAW, filtered_angles[0]));
  *pitch = clamp_angle(ltr_int_val_on_axis(PITCH, filtered_angles[1]));
  *roll = clamp_angle(ltr_int_val_on_axis(ROLL, filtered_angles[2]));
  *tx = ltr_int_val_on_axis(TX, filtered_translations[0]);
  *ty = ltr_int_val_on_axis(TY, filtered_translations[1]);
  *tz = ltr_int_val_on_axis(TZ, filtered_translations[2]);
  
  rotate_translations(heading, pitch, roll, tx, ty, tz);
  
  pthread_mutex_unlock(&pose_mutex);
/*
  log_message("%f  %f  %f\n%f  %f  %f\n\n", 
	      lt_current_pose.heading, lt_current_pose.pitch,lt_current_pose.roll,
	      lt_current_pose.tx,lt_current_pose.ty,lt_current_pose.tz);
*/
  return 0;
}


/*
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
*/

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

