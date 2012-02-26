#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "pose.h"
#include "math_utils.h"
#include <tracking.h>
#include "cal.h"
#include "utils.h"
#include "pref_global.h"

static double model_dist = 1000.0;
/* Focal length */
static double internal_focal_depth = 1000.0;

static double model_point0[3];
static double model_point1[3];
static double model_point2[3];

static double model_ref[3];

static double model_base[3][3];

static double center_ref[3] = {0.0, 0.0, 0.0};
static double center_base[3][3];

static enum {M_CAP, M_CLIP, M_SINGLE, M_FACE} type;

extern struct current_pose ltr_int_orig_pose;

static double clamp_angle(double angle);

void ltr_int_pose_init(struct reflector_model_type rm)
{
  switch(rm.type){
    case CAP:
      type = M_CAP;
      #ifdef PT_DBG
        printf("MODEL:CAP\n");
      #endif
      break;
    case CLIP:
      type = M_CLIP;
      #ifdef PT_DBG
        printf("MODEL:CLIP\n");
      #endif
      break;
    case SINGLE:
      type = M_SINGLE;
      #ifdef PT_DBG
        printf("MODEL:SINGLE\n");
      #endif
      break;
    case FACE:
      type = M_FACE;
      #ifdef PT_DBG
        printf("MODEL:FACE\n");
      #endif
      break;
    default:
      assert(0);
      break;
  }

  #ifdef PT_DBG
    printf("RM0: %g %g %g\n", rm.p0[0], rm.p0[1], rm.p0[2]);
    printf("RM1: %g %g %g\n", rm.p1[0], rm.p1[1], rm.p1[2]);
    printf("RM2: %g %g %g\n", rm.p2[0], rm.p2[1], rm.p2[2]);
    printf("RM_REF: %g %g %g\n", rm.hc[0], rm.hc[1], rm.hc[2]);
  #endif

  double ref[3];
  ref[0] = rm.hc[0];
  ref[1] = rm.hc[1];
  ref[2] = rm.hc[2];
  
  ltr_int_make_vec(rm.p0, ref, model_point0);
  ltr_int_make_vec(rm.p1, ref, model_point1);
  ltr_int_make_vec(rm.p2, ref, model_point2);

  /* Out of model points create orthonormal base */
  double vec1[3];
  double vec2[3];
  ltr_int_make_vec(model_point1, model_point0, vec1);
  ltr_int_make_vec(model_point2, model_point0, vec2);
  ltr_int_make_base(vec1, vec2, model_base);

//for testing purposes
  ltr_int_make_base(vec1, vec2, center_base);
  /* Convert reference point to model base coordinates */
//  double ref_pt[3];
  double origin[3] = {0,0,0};
  double vec3[3];
  ltr_int_make_vec(origin, model_point0, vec3);
  ltr_int_matrix_times_vec(model_base, vec3, model_ref);
  
  #ifdef MDL_DBG0
    ltr_int_print_vec(model_point0, "model_point0");
    ltr_int_print_vec(model_point1, "model_point1");
    ltr_int_print_vec(model_point2, "model_point2");
    
    ltr_int_print_matrix(model_base, "model_base");
    ltr_int_print_vec(ref, "ref");
    ltr_int_print_vec(vec3, "vec3");
    ltr_int_print_vec(model_ref, "model_ref");
  #endif
}

bool ltr_int_is_single_point()
{
  return (type == M_SINGLE) || (type == M_FACE);
}

bool ltr_int_is_face()
{
  return type == M_FACE;
}

static double blob_dist(struct blob_type b0, struct blob_type b1)
{
  return sqrt(ltr_int_sqr(b1.x-b0.x) + ltr_int_sqr(b1.y-b0.y));
}


static void iter_pose(struct bloblist_type blobs, double points[3][3], bool centering)
{
  (void) centering;
  double tmp[3];
  
  double pp0[3], pp1[3], pp2[3];
  double uv, uw, vw;
  double d, e, f;
  double d2, e2, f2;
  double a_max, a_min;
  double h, j, k, m, n, o, p, q;
  
  if(type == M_CAP){
    //CAP
    pp0[0] = blobs.blobs[0].x; pp0[1] = blobs.blobs[0].y; pp0[2] = internal_focal_depth;
    pp1[0] = blobs.blobs[1].x; pp1[1] = blobs.blobs[1].y; pp1[2] = internal_focal_depth;
    pp2[0] = blobs.blobs[2].x; pp2[1] = blobs.blobs[2].y; pp2[2] = internal_focal_depth;
    ltr_int_make_vec(model_point1, model_point2, tmp);
    d = ltr_int_vec_size(tmp);
    ltr_int_make_vec(model_point0, model_point2, tmp);
    e = ltr_int_vec_size(tmp);
    ltr_int_make_vec(model_point0, model_point1, tmp);
    f = ltr_int_vec_size(tmp);
  }else{
    //CLIP
    pp0[0] = blobs.blobs[1].x; pp0[1] = blobs.blobs[1].y; pp0[2] = internal_focal_depth;
    pp1[0] = blobs.blobs[0].x; pp1[1] = blobs.blobs[0].y; pp1[2] = internal_focal_depth;
    pp2[0] = blobs.blobs[2].x; pp2[1] = blobs.blobs[2].y; pp2[2] = internal_focal_depth;
    ltr_int_make_vec(model_point0, model_point2, tmp);
    d = ltr_int_vec_size(tmp);
    ltr_int_make_vec(model_point1, model_point2, tmp);
    e = ltr_int_vec_size(tmp);
    ltr_int_make_vec(model_point1, model_point0, tmp);
    f = ltr_int_vec_size(tmp);
  }
  
//  ltr_int_print_vec(pp0, "cp0");
//  ltr_int_print_vec(pp1, "cp1");
//  ltr_int_print_vec(pp2, "cp2");
  
  ltr_int_normalize_vec(pp0);
  ltr_int_normalize_vec(pp1);
  ltr_int_normalize_vec(pp2);

//ltr_int_print_vec(pp0, "pp0");
//ltr_int_print_vec(pp1, "pp1");
//ltr_int_print_vec(pp2, "pp2");
//  printf("d = %f\t\te = %f\t\tf = %f\n", d, e, f);
  
  uv = ltr_int_dot_product(pp0, pp1);
  uw = ltr_int_dot_product(pp0, pp2);
  vw = ltr_int_dot_product(pp1, pp2);
  d2 = d*d;
  e2 = e*e;
  f2 = f*f;
  h=uw * uw - 1.0;
  j=uv * uv - 1.0;
  
  double uxv, uxw;
  ltr_int_cross_product(pp0, pp1, tmp);
  uxv = f / ltr_int_vec_size(tmp);
  ltr_int_cross_product(pp0, pp2, tmp);
  uxw = e / ltr_int_vec_size(tmp);
  
  a_min = e < f ? f : e;
  a_max = uxv < uxw ? uxv : uxw;
  
  //printf("amin = %f; amax = %f\n", a_min, a_max);
  
  //Initial guess
  double a = a_min + (a_min + a_max) / 2;
  //printf("a = %f\n", a);
  double a2, b, c, i_d;
  int i;
  
  for(i = 0; i < 20; ++i){
    if(a < a_min + 0.1){
      a = a_min + 0.1;
    }
    if(a > a_max - 0.1){
      a = a_max - 0.1;
    }
    a2 = a * a;
    k = sqrt(a2 * j + f2);
    m = sqrt(a2 * h + e2);
    n = a * h / m;
    o = a * j / k;
    p = a * uv - k;
    q = a * uw - m;
    
    b = a * uv - k;
    c = a * uw - m;
    i_d = b * b + c * c - 2 * b * c * vw - d2;
    //printf("b = %f\nc = %f\ni_d = %f\nd = %f\n", b, c, i_d, i_d / 2*((uv-o)*(p-q*vw)+(uw-n)*(q-p*vw)));
    a -= i_d / (2*((uv-o)*(p-q*vw)+(uw-n)*(q-p*vw)));
    //printf("a = %f\n", a);
    if(abs(i_d) < 1e-2){
      break;
    }
  }
  //printf("amin = %f\namax = %f\n", a_min, a_max);
  if(type == M_CAP){
    ltr_int_mul_vec(pp0, a, points[0]);
    ltr_int_mul_vec(pp1, b, points[1]);
  }else{
    ltr_int_mul_vec(pp0, a, points[1]);
    ltr_int_mul_vec(pp1, b, points[0]);
  }
  ltr_int_mul_vec(pp2, c, points[2]);

/*   print_matrix(points, "alter92_result"); */
  #ifdef PT_DBG
    printf("RAW: %g %g %g\n", points[0][0], points[0][1], points[0][2]);
    printf("RAW: %g %g %g\n", points[1][0], points[1][1], points[1][2]);
    printf("RAW: %g %g %g\n", points[2][0], points[2][1], points[2][2]);
  #endif
}


static void alter_pose(struct bloblist_type blobs, double points[3][3], bool centering)
{
  double R01, R02, R12, d01, d02, d12, a, b, c, s, h1, h2, sigma;
  double tmp[3];

  ltr_int_make_vec(model_point0, model_point1, tmp);
  R01 = ltr_int_vec_size(tmp);
  ltr_int_make_vec(model_point0, model_point2, tmp);
  R02 = ltr_int_vec_size(tmp);
  ltr_int_make_vec(model_point1, model_point2, tmp);
  R12 = ltr_int_vec_size(tmp);

  d01 = blob_dist(blobs.blobs[0], blobs.blobs[1]);
  d02 = blob_dist(blobs.blobs[0], blobs.blobs[2]);
  d12 = blob_dist(blobs.blobs[1], blobs.blobs[2]);

  a = (R01 + R02+ R12) * (-R01 + R02 + R12)
    * (R01 - R02 + R12) * (R01 + R02 - R12);

  b = ltr_int_sqr(d01) * (-ltr_int_sqr(R01) + ltr_int_sqr(R02) + ltr_int_sqr(R12))
    + ltr_int_sqr(d02) * (ltr_int_sqr(R01) - ltr_int_sqr(R02) + ltr_int_sqr(R12))
    + ltr_int_sqr(d12) * (ltr_int_sqr(R01) + ltr_int_sqr(R02) - ltr_int_sqr(R12));

  c = (d01 + d02+ d12) * (-d01 + d02 + d12)
    * (d01 - d02 + d12) * (d01 + d02 - d12);

  s = sqrt((b + sqrt(ltr_int_sqr(b) - a * c)) / a);

  if((ltr_int_sqr(d01) + ltr_int_sqr(d02) - ltr_int_sqr(d12))
      <= (ltr_int_sqr(s) * (ltr_int_sqr(R01) + ltr_int_sqr(R02) - ltr_int_sqr(R12)))){
    sigma = 1;
  }else{
    sigma = -1;
  }

  h1 = ((type == M_CAP)? -1 : 1) * sqrt(ltr_int_sqr(s * R01) - ltr_int_sqr(d01));
  h2 = ((type == M_CAP)? -1 : 1) * sigma * sqrt(ltr_int_sqr(s * R02) - ltr_int_sqr(d02));

  if(centering){
    internal_focal_depth = model_dist * s;
  }
  
  points[0][0] = blobs.blobs[0].x / s;
  points[0][1] = blobs.blobs[0].y / s;
  points[0][2] = internal_focal_depth / s;

  points[1][0] = blobs.blobs[1].x / s;
  points[1][1] = blobs.blobs[1].y / s;
  points[1][2] = (internal_focal_depth + h1) / s;

  points[2][0] = blobs.blobs[2].x / s;
  points[2][1] = blobs.blobs[2].y / s;
  points[2][2] = (internal_focal_depth + h2) / s;

/*   print_matrix(points, "alter92_result"); */
  #ifdef PT_DBG
    printf("RAW: %g %g %g\n", points[0][0], points[1][0], points[2][0]);
    printf("RAW: %g %g %g\n", points[0][1], points[1][1], points[2][1]);
    printf("RAW: %g %g %g\n", points[0][2], points[1][2], points[2][2]);
  #endif
}


void ltr_int_pose_sort_blobs(struct bloblist_type bl)
{
  struct blob_type tmp_blob;
  int topmost_blob_index;
  /* find the topmost blob
   * so few its not worth iterating */
  if(type == M_CAP){
    topmost_blob_index = 0;
    if (bl.blobs[1].y > bl.blobs[0].y) {
      topmost_blob_index = 1;
      if (bl.blobs[2].y > bl.blobs[1].y) {
        topmost_blob_index = 2;
      }
    }
    else if (bl.blobs[2].y > bl.blobs[0].y) {
      topmost_blob_index = 2;
    }
    /* swap the topmost to index 0 */
    if (topmost_blob_index != 0) {
      tmp_blob = bl.blobs[0];
      bl.blobs[0] = bl.blobs[topmost_blob_index];
      bl.blobs[topmost_blob_index] = tmp_blob;
    }
    /* make sure the blob[1] is the leftmost */
    if (bl.blobs[2].x < bl.blobs[1].x) {
      tmp_blob = bl.blobs[1];
      bl.blobs[1] = bl.blobs[2];
      bl.blobs[2] = tmp_blob;
    }
  }else if(type == M_CLIP){
    //sort by y (bubble sort like... Hope I got it right;-)
    if (bl.blobs[1].y > bl.blobs[0].y) {
      tmp_blob = bl.blobs[1];
      bl.blobs[1] = bl.blobs[0];
      bl.blobs[0] = tmp_blob;
    }
    if (bl.blobs[2].y > bl.blobs[1].y) {
      tmp_blob = bl.blobs[1];
      bl.blobs[1] = bl.blobs[2];
      bl.blobs[2] = tmp_blob;
    }
    if (bl.blobs[1].y > bl.blobs[0].y) {
      tmp_blob = bl.blobs[1];
      bl.blobs[1] = bl.blobs[0];
      bl.blobs[0] = tmp_blob;
    }
  }
}


bool ltr_int_pose_process_blobs(struct bloblist_type blobs,
                        struct current_pose *pose, bool centering)
{
//  double points[3][3];
  double points[3][3] = {{28.35380,    -1.24458,    -0.11606},
                         {103.19049,   -44.13490,   -76.36657},
			 {131.32094,    10.75412,   -50.19649}
  };

/*
  double points[3][3] = {{22.923,   110.165,    28.070},
                         {91.241,    44.781,    91.768},
			 {184.262,    18.075,    47.793}
  };
*/
  if(ltr_int_use_alter()){
    alter_pose(blobs, points, centering);
  }else{
    iter_pose(blobs, points, centering);
  }
//ltr_int_print_matrix(points, "points");
  double new_base[3][3];
  double vec1[3];
  double vec2[3];
  ltr_int_make_vec(points[1], points[0], vec1);
  ltr_int_make_vec(points[2], points[0], vec2);
  ltr_int_make_base(vec1, vec2, new_base);
  
//  ltr_int_print_matrix(new_base, "new_base");
  if(centering == true){
    ltr_int_assign_matrix(new_base, center_base);
  }
  
  //all applications contain transposed base
  ltr_int_transpose_in_place(new_base);

  double new_center[3];
  ltr_int_matrix_times_vec(new_base, model_ref, vec1);
  ltr_int_add_vecs(points[0], vec1, new_center);
  
//  ltr_int_print_matrix(new_base, "new_base'");
//  ltr_int_print_vec(model_ref, "model_ref");
//  ltr_int_print_vec(points[0], "pt0");
//  ltr_int_print_vec(new_center, "new_center");
  
  if(centering == true){
    int i;
    for(i = 0; i < 3; ++i){
      center_ref[i] = new_center[i];
    }
  }
  double displacement[3];
  ltr_int_make_vec(new_center, center_ref, displacement);
//  ltr_int_print_vec(center_ref, "ref_pt");
//  ltr_int_print_vec(displacement, "mv");
  
//  ltr_int_print_matrix(center_base, "center_base");

  double transform[3][3];
  ltr_int_mul_matrix(new_base, center_base, transform);
//  ltr_int_print_matrix(new_base, "new_base'");
//  ltr_int_print_matrix(center_base, "center_base");
//  ltr_int_print_matrix(transform, "transform");
  double pitch, yaw, roll;
  ltr_int_matrix_to_euler(transform, &pitch, &yaw, &roll);
  pitch *= 180.0 /M_PI;
  yaw *= 180.0 /M_PI;
  roll *= 180.0 /M_PI;
//  printf("Raw Pitch: %g   Yaw: %g  Roll: %g\n", pitch, yaw, roll);
  ltr_int_orig_pose.pitch = pitch;
  ltr_int_orig_pose.heading = yaw;
  ltr_int_orig_pose.roll = roll;
  static float filterfactor=1.0;
  ltr_int_get_filter_factor(&filterfactor);
  static double filtered_angles[3] = {0.0f, 0.0f, 0.0f};
  static double filtered_translations[3] = {0.0f, 0.0f, 0.0f};
  double filter_factors_angles[3] = {filterfactor, filterfactor, filterfactor};
  double filter_factors_translations[3] = {filterfactor, filterfactor, filterfactor * 10};
  double raw_angles[3] = {pitch, yaw, roll};
  ltr_int_nonlinfilt_vec(raw_angles, filtered_angles, filter_factors_angles, filtered_angles);
  
  pose->pitch = clamp_angle(ltr_int_val_on_axis(PITCH, filtered_angles[0]));
  pose->heading = clamp_angle(ltr_int_val_on_axis(YAW, filtered_angles[1]));
  pose->roll = clamp_angle(ltr_int_val_on_axis(ROLL, filtered_angles[2]));
//  printf("Pitch: %g   Yaw: %g  Roll: %g\n", pose->pitch, pose->heading, pose->roll);
  
  double rotated[3];
//  ltr_int_euler_to_matrix(pitch / 180.0 * M_PI, yaw / 180.0 * M_PI, 
//                          roll / 180.0 * M_PI, transform);
  ltr_int_euler_to_matrix(pose->pitch / 180.0 * M_PI, pose->heading / 180.0 * M_PI, 
                          pose->roll / 180.0 * M_PI, transform);
  ltr_int_matrix_times_vec(transform, displacement, rotated);
//  ltr_int_print_matrix(transform, "trf");
//  ltr_int_print_vec(displacement, "mv");
//  ltr_int_print_vec(rotated, "rotated");
  ltr_int_nonlinfilt_vec(rotated, filtered_translations, filter_factors_translations, 
        filtered_translations);
  ltr_int_orig_pose.tx = rotated[0];
  ltr_int_orig_pose.ty = rotated[1];
  ltr_int_orig_pose.tz = rotated[2];
  pose->tx = ltr_int_val_on_axis(TX, filtered_translations[0]);
  pose->ty = ltr_int_val_on_axis(TY, filtered_translations[1]);
  pose->tz = ltr_int_val_on_axis(TZ, filtered_translations[2]);

//  ltr_int_print_vec(displacement, "tr");
//  printf("%f %f %f  %f %f %f\n", pose->pitch, pose->heading, pose->roll, pose->tx, pose->ty, pose->tz);
  return true;
}

double clamp_angle(double angle)
{
  if(angle<-180.0){
    return -180.0;
  }else if(angle>180.0){
    return 180.0;
  }else{
    return angle;
  }
}

/*
int ltr_int_pose_compute_camera_update(struct transform trans,
                               double *yaw,
                               double *pitch,
                               double *roll,
                               double *tx,
                               double *ty,
                               double *tz)
{
  double p, y, r;
  ltr_int_matrix_to_euler(trans.rot, &p, &y, &r);
  if(ltr_int_is_finite(trans.tr[0]) && ltr_int_is_finite(trans.tr[1]) 
     && ltr_int_is_finite(trans.tr[2]) && ltr_int_is_finite(p) 
     && ltr_int_is_finite(y) && ltr_int_is_finite(r)){

    *tx = trans.tr[0];
    *ty = trans.tr[1];
    *tz = trans.tr[2];
    // convert to degrees 
    (*pitch) = p * 180.0 /M_PI;
    (*yaw)   = y * 180.0 /M_PI;
    (*roll)  = r * 180.0 /M_PI;
    #ifdef PT_DBG
      printf("ROT: %g %g %g\n", *pitch, *yaw, *roll);
      printf("TRA: %g %g %g\n", *tx, *ty, *tz);
    #endif
  / *
  
    //rotate_translations(&heading_d, &pitch_d, &roll_d, &tx_d, &ty_d, &tz_d);
    *heading = heading_d;
    *pitch = pitch_d;
    *roll = roll_d;
    *tx = tx_d;
    *ty = ty_d;
    *tz = tz_d;
  * /
    return 0;
  }
  return -1;
}


int main(void)
{
  pose_init();

  struct blob_type bl[3];
  struct bloblist_type blobs;
  struct transform res;
  blobs.num_blobs = 3;
  blobs.blobs = bl;
  blobs.blobs[0].x = 0;
  blobs.blobs[0].y = 2.52632;
  blobs.blobs[1].x = -0.98824;
  blobs.blobs[1].y = 1.76471;
  blobs.blobs[2].x = 0.98824;
  blobs.blobs[2].y = 1.76471;
  pose_process_blobs(blobs ,&res);

  double pitch, roll, yaw;
  matrix_to_euler(res.rot, &pitch, &yaw, &roll);
  printf("Pitch: %f  Yaw: %f  Roll: %f\n", pitch, yaw, roll);

  return 0;
}
*/
