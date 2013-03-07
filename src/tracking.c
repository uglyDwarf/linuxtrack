#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include "tracking.h"
#include "math_utils.h"
#include "pose.h"
#include "utils.h"
#include "pref_global.h"

/**************************/
/* private Static members */
/**************************/

//static struct bloblist_type filtered_bloblist;
//static struct blob_type filtered_blobs[3];
//static bool first_frame = true;
static bool recenter = false;
static float cam_distance = 1000.0f;

pose_t ltr_int_orig_pose;

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
/************************/
/* function definitions */
/************************/

bool ltr_int_check_pose()
{
  struct reflector_model_type rm;
  if(ltr_int_model_changed(true)){
    if(!ltr_int_get_model_setup(&rm)){
      ltr_int_log_message("Can't get pose setup!\n");
      return false;
    }
    ltr_int_log_message("Initializing model!\n");
    if(!ltr_int_pose_init(rm)){
      ltr_int_log_message("Can't initialize pose!\n");
      return false;
    }
  }
  return true;
}

static bool tracking_initialized = false;
static dbg_flag_type tracking_dbg_flag = DBG_CHECK;
static dbg_flag_type raw_dbg_flag = DBG_CHECK;
static int orientation = 0;
static bool behind = false;

bool ltr_int_init_tracking()
{
  if(tracking_dbg_flag == DBG_CHECK){
    tracking_dbg_flag = ltr_int_get_dbg_flag('t');
    raw_dbg_flag = ltr_int_get_dbg_flag('r');
  }

  orientation = ltr_int_get_orientation();
  if(orientation & ORIENT_FROM_BEHIND){
    behind = true;
  }else{
    behind = false;
  }
  
  if(ltr_int_check_pose() == false){
    ltr_int_log_message("Can't get pose setup!\n");
    return false;
  }
//  filtered_bloblist.num_blobs = 3;
//  filtered_bloblist.blobs = filtered_blobs;
//  first_frame = true;
  ltr_int_log_message("Tracking initialized!\n");
  tracking_initialized = true;
  return true;
}

int ltr_int_recenter_tracking()
{
  recenter = true;
  return 0;
}


static pthread_mutex_t pose_mutex = PTHREAD_MUTEX_INITIALIZER;
static double angles[3] = {0.0f, 0.0f, 0.0f};
static double translations[3] = {0.0f, 0.0f, 0.0f};

static void ltr_int_rotate_camera(float *x, float *y, int orientation)
{
  float tmp_x;
  float tmp_y;
  if(orientation & ORIENT_XCHG_XY){
    tmp_x = *y;
    tmp_y = *x;
  }else{
    tmp_x = *x;
    tmp_y = *y;
  }
  *x = (orientation & ORIENT_FLIP_X) ? -tmp_x : tmp_x;
  *y = (orientation & ORIENT_FLIP_Y) ? -tmp_y : tmp_y;
}


static void ltr_int_remove_camera_rotation(struct bloblist_type bl)
{
  unsigned int i;
  for(i = 0; i < bl.num_blobs; ++i){
    ltr_int_rotate_camera(&(bl.blobs[i].x), &(bl.blobs[i].y), orientation);
  }
}

static int update_pose_1pt(struct frame_type *frame)
{
  static float c_x = 0.0f;
  static float c_y = 0.0f;
  static float c_z = 0.0f;
  
  if(!ltr_int_check_pose()){
    return -1;
  }
  
  if(tracking_dbg_flag == DBG_ON){
    unsigned int i;
    for(i = 0; i < frame->bloblist.num_blobs; ++i){
      ltr_int_log_message("*DBG_t* %d: %g %g %d\n", i, frame->bloblist.blobs[i].x, frame->bloblist.blobs[i].y,
                          frame->bloblist.blobs[i].score);
    }
  }
  
  ltr_int_pose_sort_blobs(frame->bloblist);
  if((frame->bloblist.num_blobs > 0) && ltr_int_is_finite(frame->bloblist.blobs[0].x) 
     && ltr_int_is_finite(frame->bloblist.blobs[0].y)){
  }else{
    return -1;
  }

  ltr_int_remove_camera_rotation(frame->bloblist);

  if(recenter){
    c_x = frame->bloblist.blobs[0].x;
    c_y = frame->bloblist.blobs[0].y;
    c_z = cam_distance * sqrtf((float)frame->bloblist.blobs[0].score);
    recenter = false;
  }
  
  double tmp_angles[3], tmp_translations[3];
  
//printf("cz = %f, z = %f\n", c_z, sqrtf((float)frame->bloblist.blobs[0].score));
  //angles will be approximately "normalized" to (-100, 100); 
  //  the rest should be handled by sensitivities
  tmp_angles[0] = (frame->bloblist.blobs[0].y - c_y) * 200.0 / frame->width;
  tmp_angles[1] = (c_x - frame->bloblist.blobs[0].x) * 200.0 / frame->width;
  tmp_angles[2] = 0.0f;
  tmp_translations[0] = 0.0f;
  tmp_translations[1] = 0.0f;
  if(ltr_int_is_face() && (frame->bloblist.blobs[0].score > 0)){
      tmp_translations[2] = c_z / sqrtf((float)frame->bloblist.blobs[0].score) - cam_distance;
  }else{
    tmp_translations[2] = 0.0f;
  }
  
  if(behind){
    tmp_angles[0] *= -1;
    //prudent, but not really needed as it is presend only in facetracking and
    //  face can't be tracked from behind ;) Well, normaly it can't ;)
    tmp_translations[2] *= -1;
  }
  
  int i;
  pthread_mutex_lock(&pose_mutex);
  for(i = 0; i < 3; ++i){
    angles[i] = tmp_angles[i];
    translations[i] = tmp_translations[i];
  }
  pthread_mutex_unlock(&pose_mutex);
  return 0;
}


static int update_pose_3pt(struct frame_type *frame)
{
  if(ltr_int_model_changed(false)){
    ltr_int_check_pose();
    recenter = true;
  }
  
  if(frame->bloblist.num_blobs != 3){
    return -1;
  }
  if(ltr_int_is_finite(frame->bloblist.blobs[0].x) && ltr_int_is_finite(frame->bloblist.blobs[0].y) &&
      ltr_int_is_finite(frame->bloblist.blobs[1].x) && ltr_int_is_finite(frame->bloblist.blobs[1].y) &&
      ltr_int_is_finite(frame->bloblist.blobs[2].x) && ltr_int_is_finite(frame->bloblist.blobs[2].y)){
  }else{
    return -1;
  }
  if(tracking_dbg_flag == DBG_ON){
    unsigned int i;
    for(i = 0; i < frame->bloblist.num_blobs; ++i){
      ltr_int_log_message("*DBG_t* %d: %g %g %d\n", i, frame->bloblist.blobs[i].x, frame->bloblist.blobs[i].y,
                          frame->bloblist.blobs[i].score);
    }
  }
  
  ltr_int_remove_camera_rotation(frame->bloblist);
  ltr_int_pose_sort_blobs(frame->bloblist);

  pose_t t;
  if(!ltr_int_pose_process_blobs(frame->bloblist, &t, recenter)){
    return -1;
  }
  double tmp_angles[3], tmp_translations[3];
  

  angles[0] = t.pitch;
  angles[1] = t.yaw;
  angles[2] = t.roll;
  translations[0] = t.tx;
  translations[1] = t.ty;
  translations[2] = t.tz;

  if(!ltr_int_is_vector_finite(tmp_angles) || !ltr_int_is_vector_finite(tmp_translations)){
    return false;
  }

  if(behind){
    tmp_angles[0] *= -1;
    tmp_angles[2] *= -1;
    tmp_translations[0] *= -1;
    tmp_translations[2] *= -1;
  }

  int i;
  pthread_mutex_lock(&pose_mutex);
  for(i = 0; i < 3; ++i){
    angles[i] = tmp_angles[i];
    translations[i] = tmp_translations[i];
  }
  pthread_mutex_unlock(&pose_mutex);
  if(raw_dbg_flag == DBG_ON){
    printf("*DBG_r* yaw: %g pitch: %g roll: %g\n", angles[0], angles[1], angles[2]);
    ltr_int_log_message("*DBG_r* x: %g y: %g z: %g\n", 
                        translations[0], translations[1], translations[2]);
  }
  
  return 0;
}

bool ltr_int_postprocess_axes(ltr_axes_t axes, pose_t *pose, pose_t *unfiltered)
{
//  printf(">>Pre: %f %f %f  %f %f %f\n", pose->pitch, pose->yaw, pose->roll, pose->tx, pose->ty, pose->tz);
//  static float filterfactor=1.0;
//  ltr_int_get_filter_factor(&filterfactor);
  static float filtered_angles[3] = {0.0f, 0.0f, 0.0f};
  static float filtered_translations[3] = {0.0f, 0.0f, 0.0f};
  //ltr_int_get_axes_ff(axes, filter_factors);
  double raw_angles[3];
  
  //Single point must be "denormalized"
  
  
  
  raw_angles[0] = unfiltered->pitch = ltr_int_val_on_axis(axes, PITCH, pose->pitch);
  raw_angles[1] = unfiltered->yaw = ltr_int_val_on_axis(axes, YAW, pose->yaw);
  raw_angles[2] = unfiltered->roll = ltr_int_val_on_axis(axes, ROLL, pose->roll);
  //printf(">>Raw: %f %f %f\n", raw_angles[0], raw_angles[1], raw_angles[2]);
  
  if(!ltr_int_is_vector_finite(raw_angles)){
    return false;
  }
  
  pose->pitch = clamp_angle(ltr_int_filter_axis(axes, PITCH, raw_angles[0], &(filtered_angles[0])));
  pose->yaw = clamp_angle(ltr_int_filter_axis(axes, YAW, raw_angles[1], &(filtered_angles[1])));
  pose->roll = clamp_angle(ltr_int_filter_axis(axes, ROLL, raw_angles[2], &(filtered_angles[2])));
  
  double rotated[3];
  double transform[3][3];
  double displacement[3] = {pose->tx, pose->ty, pose->tz};
  if(ltr_int_do_tr_align()){
    //printf("Translations: Aligned\n");
    ltr_int_euler_to_matrix(pose->pitch / 180.0 * M_PI, pose->yaw / 180.0 * M_PI, 
                            pose->roll / 180.0 * M_PI, transform);
    ltr_int_matrix_times_vec(transform, displacement, rotated);
//  ltr_int_print_matrix(transform, "trf");
//  ltr_int_print_vec(displacement, "mv");
//  ltr_int_print_vec(rotated, "rotated");
    unfiltered->tx = ltr_int_val_on_axis(axes, TX, rotated[0]);
    unfiltered->ty = ltr_int_val_on_axis(axes, TY, rotated[1]);
    unfiltered->tz = ltr_int_val_on_axis(axes, TZ, rotated[2]);
    if(!ltr_int_is_vector_finite(rotated)){
      return false;
    }
  }else{
    //printf("Translations: Unaligned\n");
    unfiltered->tx = ltr_int_val_on_axis(axes, TX, displacement[0]);
    unfiltered->ty = ltr_int_val_on_axis(axes, TY, displacement[1]);
    unfiltered->tz = ltr_int_val_on_axis(axes, TZ, displacement[2]);
  }

  pose->tx = 
    ltr_int_filter_axis(axes, TX, unfiltered->tx, &(filtered_translations[0]));
  pose->ty = 
    ltr_int_filter_axis(axes, TY, unfiltered->ty, &(filtered_translations[1]));
  pose->tz = 
    ltr_int_filter_axis(axes, TZ, unfiltered->tz, &(filtered_translations[2]));
  //printf(">>Post: %f %f %f  %f %f %f\n", pose->pitch, pose->yaw, pose->roll, pose->tx, pose->ty, pose->tz);
  return true;
}


static uint32_t counter_d = 0;

int ltr_int_update_pose(struct frame_type *frame)
{
  bool res;
  if(ltr_int_is_single_point()){
    res =  update_pose_1pt(frame);
  }else{
    res = update_pose_3pt(frame);
  }
  if(res){
    ++counter_d;
  }
  return res;
}

int ltr_int_tracking_get_camera(float *heading,
                      float *pitch,
                      float *roll,
                      float *tx,
                      float *ty,
                      float *tz,
                      uint32_t *counter)
{
  if(!tracking_initialized){
    ltr_int_init_tracking();
  }
  
  pthread_mutex_lock(&pose_mutex);
  *pitch = angles[0];
  *heading = angles[1];
  *roll = angles[2];
  *tx = translations[0];
  *ty = translations[1];
  *tz = translations[2];
  
  *counter = counter_d;
  pthread_mutex_unlock(&pose_mutex);
  return 0;
}

