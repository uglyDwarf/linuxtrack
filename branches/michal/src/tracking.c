#include "tracking.h"
#include "math_utils.h"
#include "pose.h"
#include "utils.h"
#include "pref_global.h"

static float filterfactor=1.0;

/**************************/
/* private Static members */
/**************************/

static struct lt_axes axes;
static struct bloblist_type filtered_bloblist;
static struct blob_type filtered_blobs[3];
static bool first_frame = true;
static bool recenter = false;
static bool axes_changed = false;
struct current_pose lt_current_pose;
struct current_pose lt_orig_pose;

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
  if(model_changed()){
    if(get_model_setup(&rm) == false){
      log_message("Can't get pose setup!\n");
      return false;
    }
    pose_init(rm);
  }
  return true;
}

static bool get_axes()
{
  bool res = true;
  res &= get_axis("Pitch", &(axes.pitch_axis), &axes_changed);
  res &= get_axis("Yaw", &(axes.yaw_axis), &axes_changed);
  res &= get_axis("Roll", &(axes.roll_axis), &axes_changed);
  res &= get_axis("Xtranslation", &(axes.tx_axis), &axes_changed);
  res &= get_axis("Ytranslation", &(axes.ty_axis), &axes_changed);
  res &= get_axis("Ztranslation", &(axes.tz_axis), &axes_changed);
  return res;
}

static void close_axes()
{
  close_axis(&(axes.pitch_axis));
  close_axis(&(axes.yaw_axis));
  close_axis(&(axes.roll_axis));
  close_axis(&(axes.tx_axis));
  close_axis(&(axes.ty_axis));
  close_axis(&(axes.tz_axis));
}

bool init_tracking()
{
  if(get_filter_factor(&filterfactor) != true){
    return false;
  }
  if(!get_axes()){
    log_message("Couldn't load axes definitions!\n");
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

static int update_pose_1pt(struct frame_type *frame)
{
  static float filtered_x, filtered_y;
  static float c_x, c_y;
  bool recentering = false;
  check_pose();
  
  if(recenter == true){
    recenter = false;
    recentering = true;
    log_message("RECENTERING!\n");
  } 
  
  get_filter_factor(&filterfactor);
  
  if(is_finite(frame->bloblist.blobs[0].x) && is_finite(frame->bloblist.blobs[0].y)){
  }else{
    return -1;
  }

  if (first_frame || recentering) {
    filtered_x = frame->bloblist.blobs[0].x;
    filtered_y = frame->bloblist.blobs[0].y;
  }else{
    filtered_x = nonlinfilt(frame->bloblist.blobs[0].x, filtered_x, filterfactor);
    filtered_y = nonlinfilt(frame->bloblist.blobs[0].y, filtered_y, filterfactor);
  }
  if (first_frame) {
    first_frame = false;
  }
  if(recentering){
    c_x = filtered_x;
    c_y = filtered_y;
  }
  
  if(axes_changed){
    axes_changed = false;
    close_axes();
    get_axes();
  }
  
  pthread_mutex_lock(&pose_mutex);
  
  lt_orig_pose.heading = filtered_x - c_x;
  lt_orig_pose.pitch = filtered_y - c_y;
  lt_orig_pose.roll = 0.0f;
  lt_orig_pose.tx = 0.0f;
  lt_orig_pose.ty = 0.0f;
  lt_orig_pose.tz = 0.0f;
  
  lt_current_pose.heading = clamp_angle(val_on_axis(axes.yaw_axis, filtered_x - c_x));
  lt_current_pose.pitch = clamp_angle(val_on_axis(axes.pitch_axis, filtered_y - c_y));
  lt_current_pose.roll = 0.0f;
  lt_current_pose.tx = 0.0f;
  lt_current_pose.ty = 0.0f;
  lt_current_pose.tz = 0.0f;
  
  pthread_mutex_unlock(&pose_mutex);
  return 0;

}

static int update_pose_3pt(struct frame_type *frame)
{
  float raw_angles[3];
  float raw_translations[3];
  static float filtered_angles[3] = {0.0f, 0.0f, 0.0f};
  static float filtered_translations[3] = {0.0f, 0.0f, 0.0f};
  struct transform t;
  bool recentering = false;
  check_pose();

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
      
/*  log_message("[%f,%f], [%f, %f], [%f, %f]\n", filtered_bloblist.blobs[0].x, 
  filtered_bloblist.blobs[0].y,  filtered_bloblist.blobs[1].x,
  filtered_bloblist.blobs[1].y, filtered_bloblist.blobs[2].x,
  filtered_bloblist.blobs[2].y);
*/
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
  
  if(axes_changed){
    axes_changed = false;
    close_axes();
    get_axes();
  }
  
  pthread_mutex_lock(&pose_mutex);
  
  lt_orig_pose.heading = filtered_angles[0];
  lt_orig_pose.pitch = filtered_angles[1];
  lt_orig_pose.roll = filtered_angles[2];
  lt_orig_pose.tx = filtered_translations[0];
  lt_orig_pose.ty = filtered_translations[1];
  lt_orig_pose.tz = filtered_translations[2];
  
  lt_current_pose.heading = clamp_angle(val_on_axis(axes.yaw_axis, filtered_angles[0]));
  lt_current_pose.pitch = clamp_angle(val_on_axis(axes.pitch_axis, filtered_angles[1]));
  lt_current_pose.roll = clamp_angle(val_on_axis(axes.roll_axis, filtered_angles[2]));
  lt_current_pose.tx = val_on_axis(axes.tx_axis, filtered_translations[0]);
  lt_current_pose.ty = val_on_axis(axes.ty_axis, filtered_translations[1]);
  lt_current_pose.tz = val_on_axis(axes.tz_axis, filtered_translations[2]);
  
  rotate_translations(&lt_current_pose.heading, &lt_current_pose.pitch, 
                      &lt_current_pose.roll, &lt_current_pose.tx,
		      &lt_current_pose.ty, &lt_current_pose.tz);
  pthread_mutex_unlock(&pose_mutex);
  return 0;
}

int update_pose(struct frame_type *frame)
{
  if(is_single_point()){
    return update_pose_1pt(frame);
  }else{
    return update_pose_3pt(frame);
  }
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

