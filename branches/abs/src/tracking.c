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
static bool init_recenter = false;
static float cam_distance = 1000.0f;

static linuxtrack_full_pose_t current_pose;

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
  init_recenter = true;
  recenter = false;
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

static void ltr_int_rotate_camera(float *x, float *y, int cam_orientation)
{
  float tmp_x;
  float tmp_y;
  if(cam_orientation & ORIENT_XCHG_XY){
    tmp_x = *y;
    tmp_y = *x;
  }else{
    tmp_x = *x;
    tmp_y = *y;
  }
  *x = (cam_orientation & ORIENT_FLIP_X) ? -tmp_x : tmp_x;
  *y = (cam_orientation & ORIENT_FLIP_Y) ? -tmp_y : tmp_y;
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

  //printf("Updating pose 1pt...\n");
  if(!ltr_int_check_pose()){
    return -1;
  }

  //printf("Updating pose...\n");
  if(tracking_dbg_flag == DBG_ON){
    unsigned int i;
    for(i = 0; i < frame->bloblist.num_blobs; ++i){
      ltr_int_log_message("*DBG_t* %d: %g %g %d\n", i, frame->bloblist.blobs[i].x, frame->bloblist.blobs[i].y,
                          frame->bloblist.blobs[i].score);
    }
  }

  if((frame->bloblist.num_blobs > 0) && ltr_int_is_finite(frame->bloblist.blobs[0].x)
     && ltr_int_is_finite(frame->bloblist.blobs[0].y)){
  }else{
    return -1;
  }

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

  pthread_mutex_lock(&pose_mutex);
  current_pose.pose.raw_pitch = tmp_angles[0];
  current_pose.pose.raw_yaw = tmp_angles[1];
  current_pose.pose.raw_roll = tmp_angles[2];
  current_pose.pose.raw_tx = tmp_translations[0];
  current_pose.pose.raw_ty = tmp_translations[1];
  current_pose.pose.raw_tz = tmp_translations[2];
  pthread_mutex_unlock(&pose_mutex);
  //printf("Pose updated => rp: %g, ry: %g...\n", current_pose.raw_pitch, current_pose.raw_yaw);
  return 0;
}


static float two_d_size(struct blob_type b1, struct blob_type b2)
{
  float d1 = b1.x - b2.x;
  float d2 = b1.y - b2.y;
  return sqrtf((d1 * d1) + (d2 * d2));
}


static int update_pose_3pt(struct frame_type *frame)
{
  if(frame->bloblist.num_blobs < 3){
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
    ltr_int_log_message("*DBG_t* d1 = %g   d2 = %g   d3 = %g\n",
      two_d_size(frame->bloblist.blobs[0], frame->bloblist.blobs[1]),
      two_d_size(frame->bloblist.blobs[0], frame->bloblist.blobs[2]),
      two_d_size(frame->bloblist.blobs[1], frame->bloblist.blobs[2]));
  }

  linuxtrack_pose_t t;
  if(!ltr_int_pose_process_blobs(frame->bloblist, &t, recenter)){
    return -1;
  }
  recenter = false;
  double tmp_angles[3], tmp_translations[3];


  tmp_angles[0] = t.pitch;
  tmp_angles[1] = t.yaw;
  tmp_angles[2] = t.roll;
  tmp_translations[0] = t.tx;
  tmp_translations[1] = t.ty;
  tmp_translations[2] = t.tz;

  if(!ltr_int_is_vector_finite(tmp_angles) || !ltr_int_is_vector_finite(tmp_translations)){
    return -1;
  }

  if(behind){
    tmp_angles[0] *= -1;
    tmp_angles[2] *= -1;
    tmp_translations[0] *= -1;
    tmp_translations[2] *= -1;
  }

  pthread_mutex_lock(&pose_mutex);
  current_pose.pose.pitch = tmp_angles[0];
  current_pose.pose.yaw = tmp_angles[1];
  current_pose.pose.roll = tmp_angles[2];
  current_pose.pose.tx = tmp_translations[0];
  current_pose.pose.ty = tmp_translations[1];
  current_pose.pose.tz = tmp_translations[2];
  current_pose.pose.raw_pitch = t.raw_pitch;
  current_pose.pose.raw_yaw = t.raw_yaw;
  current_pose.pose.raw_roll = t.raw_roll;
  current_pose.pose.raw_tx = t.raw_tx;
  current_pose.pose.raw_ty = t.raw_ty;
  current_pose.pose.raw_tz = t.raw_tz;
  pthread_mutex_unlock(&pose_mutex);
  if(raw_dbg_flag == DBG_ON){
    printf("*DBG_r* yaw: %g pitch: %g roll: %g\n", angles[0], angles[1], angles[2]);
    ltr_int_log_message("*DBG_r* x: %g y: %g z: %g\n",
                        translations[0], translations[1], translations[2]);
  }

  return 0;
}

bool ltr_int_postprocess_axes(ltr_axes_t axes, linuxtrack_pose_t *pose, linuxtrack_pose_t *unfiltered)
{
//  printf(">>Pre: %f %f %f  %f %f %f\n", pose->raw_pitch, pose->raw_yaw, pose->raw_roll,
//         pose->raw_tx, pose->raw_ty, pose->raw_tz);
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

  double displacement[3];
  displacement[0] = ltr_int_val_on_axis(axes, TX, pose->tx);
  displacement[1] = ltr_int_val_on_axis(axes, TY, pose->ty);
  displacement[2] = ltr_int_val_on_axis(axes, TZ, pose->tz);
  if(ltr_int_do_tr_align()){
    //printf("Translations: Aligned\n");
    ltr_int_euler_to_matrix(pose->pitch / 180.0 * M_PI, pose->yaw / 180.0 * M_PI,
                            pose->roll / 180.0 * M_PI, transform);
    ltr_int_matrix_times_vec(transform, displacement, rotated);
    if(!ltr_int_is_vector_finite(rotated)){
      return false;
    }
//  ltr_int_print_matrix(transform, "trf");
//  ltr_int_print_vec(displacement, "mv");
//  ltr_int_print_vec(rotated, "rotated");
    unfiltered->tx = rotated[0];
    unfiltered->ty = rotated[1];
    unfiltered->tz = rotated[2];
  }else{
    //printf("Translations: Unaligned\n");
    unfiltered->tx = displacement[0];
    unfiltered->ty = displacement[1];
    unfiltered->tz = displacement[2];
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
  //printf("Updating pose...\n");
  if(ltr_int_model_changed(false)){
    ltr_int_check_pose();
    recenter = true;
  }
  unsigned int i;
  ltr_int_remove_camera_rotation(frame->bloblist);
  ltr_int_pose_sort_blobs(frame->bloblist);
  pthread_mutex_lock(&pose_mutex);
  current_pose.pose.resolution_x = frame->width;
  current_pose.pose.resolution_y = frame->height;
  for(i = 0; i < MAX_BLOBS * BLOB_ELEMENTS; ++i){
    current_pose.blob_list[i] = 0.0;
  }
  for(i = 0; i < frame->bloblist.num_blobs; ++i){
    current_pose.blob_list[i * BLOB_ELEMENTS] = frame->bloblist.blobs[i].x;
    current_pose.blob_list[i * BLOB_ELEMENTS + 1] = frame->bloblist.blobs[i].y;
    current_pose.blob_list[i * BLOB_ELEMENTS + 2] = frame->bloblist.blobs[i].score;
  }
  current_pose.blobs = frame->bloblist.num_blobs;

  pthread_mutex_unlock(&pose_mutex);
  bool res = -1;
  if(ltr_int_is_single_point()){
    res = update_pose_1pt(frame);
  }else{
    res = update_pose_3pt(frame);
  }
  if(res == 0){
    ++counter_d;
    if(init_recenter){
      //discard the first valid frame to allow good recenter on the next one
      init_recenter = false;
      recenter = true;
      --counter_d;
      res = -1;
    }
  }
  return res;
}

int ltr_int_tracking_get_pose(linuxtrack_full_pose_t *pose)
{
  if(!tracking_initialized){
    ltr_int_init_tracking();
  }

  pthread_mutex_lock(&pose_mutex);
  current_pose.pose.status = pose->pose.status;
  pose->pose = current_pose.pose;
  pose->pose.counter = counter_d;
  pose->blobs = current_pose.blobs;
  int i;
  for(i = 0; i < (int)current_pose.blobs * BLOB_ELEMENTS; ++i){
    pose->blob_list[i] = current_pose.blob_list[i];
  }
  pthread_mutex_unlock(&pose_mutex);
  return 0;
}

