#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "pose.h"
#include "math_utils.h"
#include "tracking.h"
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
static bool use_alter = false;
static bool use_old_pose = false;


static double c_base[3];
static double tr_center[3];
static double tr_rot[3][3];

bool ltr_int_center(double rp0[3], double rp1[3], double rp2[3], double c_base[3], 
  double center[3], double tr[3][3]);
bool ltr_int_get_cbase(double p0[3], double p1[3], double p2[3], double c[3], 
  double c_base[3]);
bool ltr_int_get_pose(double rp0[3], double rp1[3], double rp2[3], double c_base[3],
  double center[3], double tr[3][3], double angles[3], double trans[3]);

bool ltr_int_pose_init(struct reflector_model_type rm)
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
      return true;
      break;
    case FACE:
      type = M_FACE;
      #ifdef PT_DBG
        printf("MODEL:FACE\n");
      #endif
      return true;
      break;
    default:
      ltr_int_log_message("Unknown model type specified %d!\n", rm.type);
      assert(0);
      return false;
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
  double vec1[3] = {0.0, 0.0, 0.0};
  double vec2[3] = {0.0, 0.0, 0.0};
  ltr_int_make_vec(model_point1, model_point0, vec1);
  ltr_int_make_vec(model_point2, model_point0, vec2);
  ltr_int_make_base(vec1, vec2, model_base);
  if(!ltr_int_is_matrix_finite(model_base)){
    ltr_int_log_message("Incorrect model dimmensions - can't create orthonormal base!\n");
    return false;
  }

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
  use_alter = ltr_int_use_alter();
  use_old_pose = ltr_int_use_oldrot();
  return ltr_int_get_cbase(rm.p0, rm.p1, rm.p2, rm.hc, c_base) &&
    ltr_int_center(rm.p0, rm.p1, rm.p2, c_base, tr_center, tr_rot);
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
  }else if(ltr_int_is_single_point()){
    //Get biggest one to index 0
    if(bl.blobs[0].score > bl.blobs[1].score){
      if(bl.blobs[0].score > bl.blobs[2].score){
        // 0 is biggest already
      }else{
        tmp_blob = bl.blobs[0];
        bl.blobs[0] = bl.blobs[2];
        bl.blobs[2] = tmp_blob;
      }
    }else{
      if(bl.blobs[1].score > bl.blobs[2].score){
        tmp_blob = bl.blobs[0];
        bl.blobs[0] = bl.blobs[1];
        bl.blobs[1] = tmp_blob;
      }else{
        tmp_blob = bl.blobs[0];
        bl.blobs[0] = bl.blobs[2];
        bl.blobs[2] = tmp_blob;
      }
    }
  }
}


bool ltr_int_pose_process_blobs(struct bloblist_type blobs,
                        pose_t *pose, bool centering)
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
  if(use_alter){
    alter_pose(blobs, points, centering);
  }else{
    iter_pose(blobs, points, centering);
  }
  
  double angles[3];
  double displacement[3];
  if(use_old_pose){
  //ltr_int_print_matrix(points, "points");
    double new_base[3][3];
    double vec1[3];
    double vec2[3];
    ltr_int_make_vec(points[1], points[0], vec1);
    ltr_int_make_vec(points[2], points[0], vec2);
    ltr_int_make_base(vec1, vec2, new_base);
    
  //  ltr_int_print_matrix(new_base, "new_base");
    if(!ltr_int_is_matrix_finite(new_base)){
      return false;
    }
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
    ltr_int_make_vec(new_center, center_ref, displacement);
  //  ltr_int_print_vec(center_ref, "ref_pt");
  //  ltr_int_print_vec(displacement, "mv");
    
  //  ltr_int_print_matrix(center_base, "center_base");
    
    double transform[3][3];
    ltr_int_mul_matrix(new_base, center_base, transform);
  //  ltr_int_print_matrix(new_base, "new_base'");
  //  ltr_int_print_matrix(center_base, "center_base");
  //  ltr_int_print_matrix(transform, "transform");
    //double pitch, yaw, roll;
    ltr_int_matrix_to_euler(transform, &(angles[0]), &(angles[1]), &(angles[2]));
  }else{
    if(centering){
      if(!ltr_int_center(points[0], points[1], points[2], c_base, tr_center, tr_rot)){
        ltr_int_log_message("Couldn't center in new pose!\n");
        return false;
      }
    }
    if(!ltr_int_get_pose(points[0], points[1], points[2], c_base, tr_center, 
                         tr_rot, angles, displacement)){
      ltr_int_log_message("Couldn't determine the pose in new pose!\n");
      return false;
    }
  }
  
  ltr_int_mul_vec(angles, 180.0 /M_PI, angles);
  
//  printf("Raw Pitch: %g   Yaw: %g  Roll: %g\n", pitch, yaw, roll);
  if(ltr_int_is_vector_finite(angles) && ltr_int_is_vector_finite(displacement)){
    pose->pitch = angles[0];
    pose->yaw = angles[1];
    pose->roll = angles[2];
    pose->tx = displacement[0];
    pose->ty = displacement[1];
    pose->tz = displacement[2];
    
    return true;
  }
  return false;
}

//Determine coordinates of the model's center of rotation in its local coordinates
//  allowing to find it given only its three points in space.
bool ltr_int_get_cbase(double p0[3], double p1[3], double p2[3], double c[3], 
  double c_base[3])
{
  double R[3][3];
  double base[3][3];
  //move model's center of rotation to [0,0,0]
  ltr_int_make_vec(p0, c, R[0]);
  ltr_int_make_vec(p1, c, R[1]);
  ltr_int_make_vec(p2, c, R[2]);
  
  //create a local model's orthonormal base (center is p0)
  double b1[3], b2[3];
  ltr_int_make_vec(R[1], R[0], b1);
  ltr_int_make_vec(R[2], R[0], b2);
  ltr_int_make_base(b1, b2, base);
  if(!ltr_int_is_matrix_finite(base)){
    return false;
  }
  //devise center of rotation in model's local base coordinates
  //  this allows to determine model's center rotation given only rp0-rp2
  double tmp[3] = {0,0,0};
  //reverse the vector to first point - later when added to first point,
  //  it gives the center's location
  ltr_int_make_vec(tmp, R[0], tmp);
  ltr_int_matrix_times_vec(base, tmp, c_base);
  return true;
}


//Employed when centering to determine center position and transformation
//  cancelling the rotation
bool ltr_int_center(double rp0[3], double rp1[3], double rp2[3], double c_base[3], 
  double center[3], double tr[3][3])
{
  double rb0[3], rb1[3], rb[3][3], rc[3], RR[3][3];
  
  //find model's current local base
  ltr_int_make_vec(rp1, rp0, rb0);
  ltr_int_make_vec(rp2, rp0, rb1);
  ltr_int_make_base(rb0, rb1, rb);
  ltr_int_transpose_in_place(rb);
  
  if(!ltr_int_is_matrix_finite(rb)){
    return false;
  }

  //ltr_int_print_matrix(rb, "rb");
  //transform model's center from local to global coordinates
  //  (using p0 as an anchor)
  ltr_int_matrix_times_vec(rb, c_base, rc);
  ltr_int_add_vecs(rc, rp0, center);
  
  
  //Devise the reverse transformation
  //  transform zero to the model's center of rotation
  
  //ltr_int_print_vec(center, "center");
  
  ltr_int_make_vec(rp0, center, RR[0]);
  ltr_int_make_vec(rp1, center, RR[1]);
  ltr_int_make_vec(rp2, center, RR[2]);
  ltr_int_transpose_in_place(RR);
  ltr_int_print_matrix(RR, "RR");
  //get inverse transform (not orthonormal!!!)
  double tmp_tr[3][3];
  ltr_int_invert_matrix(RR, tmp_tr);
  if(!ltr_int_is_matrix_finite(tmp_tr)){
    return false;
  }
  ltr_int_assign_matrix(tmp_tr, tr);
  return true;
}

bool ltr_int_get_pose(double rp0[3], double rp1[3], double rp2[3], double c_base[3],
  double center[3], double tr[3][3], double angles[3], double trans[3])
{
  double rb0[3], rb1[3], rb[3][3], rc[3], RR[3][3];
  
  //find model's current local base
  ltr_int_make_vec(rp1, rp0, rb0);
  ltr_int_make_vec(rp2, rp0, rb1);
  ltr_int_make_base(rb0, rb1, rb);
  ltr_int_transpose_in_place(rb);
  if(!ltr_int_is_matrix_finite(rb)){
    return false;
  }
  
  //ltr_int_print_matrix(rb, "rb");
  //transform model's center from local to global coordinates
  //  (using p0 as an anchor)  
  ltr_int_matrix_times_vec(rb, c_base, rc);
  double current_center[3];
  
  ltr_int_add_vecs(rc, rp0, current_center);
  ltr_int_make_vec(current_center, center, trans);
  
  double rot[3][3];
  ltr_int_make_vec(rp0, current_center, RR[0]);
  ltr_int_make_vec(rp1, current_center, RR[1]);
  ltr_int_make_vec(rp2, current_center, RR[2]);
  ltr_int_transpose_in_place(RR);
  ltr_int_mul_matrix(RR, tr, rot);
  if(!ltr_int_is_matrix_finite(rot)){
    return false;
  }
  ltr_int_matrix_to_euler(rot, &(angles[0]), &(angles[1]), &(angles[2]));
  return true;
}

