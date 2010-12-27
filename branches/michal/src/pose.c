#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "pose.h"
#include "math_utils.h"
#include "cal.h"
#include "utils.h"

static double model_dist = 1000.0;
/* Focal length */
static double internal_focal_depth;

static double model_point0[3];
static double model_point1[3];
static double model_point2[3];

static double model_ref[3];

static double model_base[3][3];

static double center_ref[3];
static double center_base[3][3];

static enum {M_CAP, M_CLIP, M_SINGLE} type;

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
    default:
      assert(0);
      break;
  }
  /* Physical dimensions */
  /* Camera looks in direction of Z axis */
  /* X axis goes then to the right */
  /* Y axis goes up */
/*   double x = 168; */
/*   double y = 90; */
/*   double z = 100; */

/*   /\* Point, around which model rotates (just dimensions)*\/ */
/*   double ref_y = 150; */
/*   double ref_z = 150; */

/*   model_point0[0] = 0.0;  */
/*   model_point0[1] = y;  */
/*   model_point0[2] = z; */

  #ifdef PT_DBG
    printf("RM1: %g %g %g\n", rm.p1[0], rm.p1[1], rm.p1[2]);
    printf("RM2: %g %g %g\n", rm.p2[0], rm.p2[1], rm.p2[2]);
    printf("RM_REF: %g %g %g\n", rm.hc[0], rm.hc[1], rm.hc[2]);
  #endif

  model_point0[0] = 0.0;
  model_point0[1] = 0.0;
  model_point0[2] = 0.0;

/*   model_point1[0] = -x/2;  */
/*   model_point1[1] = 0.0;  */
/*   model_point1[2] = 0.0; */
  model_point1[0] = rm.p1[0];
  model_point1[1] = rm.p1[1];
  model_point1[2] = rm.p1[2];

/*   model_point2[0] = x/2;  */
/*   model_point2[1] = 0.0;  */
/*   model_point2[2] = 0.0; */
  model_point2[0] = rm.p2[0];
  model_point2[1] = rm.p2[1];
  model_point2[2] = rm.p2[2];

/*   ref[0] = 0.0;  */
/*   ref[1] = -ref_y;  */
/*   ref[2] = ref_z; */
  double ref[3];
  ref[0] = rm.hc[0];
  ref[1] = rm.hc[1];
  ref[2] = rm.hc[2];

  
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
  double vec3[3];
  ltr_int_make_vec(ref, model_point0, vec3);
  ltr_int_matrix_times_vec(model_base, vec3, model_ref);
}

bool ltr_int_is_single_point()
{
  return type == M_SINGLE;
}

static double blob_dist(struct blob_type b0, struct blob_type b1)
{
  return sqrt(ltr_int_sqr(b1.x-b0.x) + ltr_int_sqr(b1.y-b0.y));
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
  points[1][0] = blobs.blobs[0].y / s;
  points[2][0] = internal_focal_depth / s;

  points[0][1] = blobs.blobs[1].x / s;
  points[1][1] = blobs.blobs[1].y / s;
  points[2][1] = (internal_focal_depth + h1) / s;

  points[0][2] = blobs.blobs[2].x / s;
  points[1][2] = blobs.blobs[2].y / s;
  points[2][2] = (internal_focal_depth + h2) / s;

/*   print_matrix(points, "alter92_result"); */
  #ifdef PT_DBG
    printf("RAW: %g %g %g\n", points[0][0], points[1][0], points[2][0]);
    printf("RAW: %g %g %g\n", points[0][1], points[1][1], points[2][1]);
    printf("RAW: %g %g %g\n", points[0][2], points[1][2], points[2][2]);
  #endif
}

static void get_translation(double base[3][3], double ref[3], double origin[3],
                     double trans[3], bool do_center){
//  double tmp[3];
  double t_base[3][3];
  double new_ref[3];
  ltr_int_transpose(base, t_base);
  ltr_int_matrix_times_vec(t_base, ref, new_ref);
  new_ref[0] += origin[0];
  new_ref[1] += origin[1];
  new_ref[2] += origin[2];
//  ltr_int_print_vec(ref, "ref");
//  ltr_int_print_vec(new_ref, "new_ref");
//  ltr_int_print_vec(origin, "origin");
//  ltr_int_print_vec(center_ref, "center_ref");
  if(do_center == true){
    center_ref[0] = new_ref[0];
    center_ref[1] = new_ref[1];
    center_ref[2] = new_ref[2];
  }
  trans[0] = new_ref[0] - center_ref[0];
  trans[1] = new_ref[1] - center_ref[1];
  trans[2] = new_ref[2] - center_ref[2];
}

static void get_transform(double new_base[3][3], double rot[3][3]){
  double center_base_t[3][3];
  ltr_int_transpose(center_base, center_base_t);
  ltr_int_mul_matrix(center_base_t, new_base, rot);
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
                        struct transform *trans, bool centering)
{

  double points[3][3];
/*   /\* DELETEME TEST START *\/ */
/*   printf("deleteme test start\n"); */
/*   struct bloblist_type testbloblist; */
/*   testbloblist.num_blobs = 3; */
/*   struct blob_type test_b[3]; */
/*   test_b[0].x= -32.370056; */
/*   test_b[0].y= 42.453735; */
/*   test_b[0].score= 227; */
/*   test_b[1].x= -69.601227; */
/*   test_b[1].y= -53.773010; */
/*   test_b[1].score= 163; */
/*   test_b[2].x= -149.311951; */
/*   test_b[2].y= -45.789001; */
/*   test_b[2].score = 109; */
/*   testbloblist.blobs = test_b; */
/*   printf("unsorted_blobs: \n"); */
/*   bloblist_print(testbloblist); */
/*   sort_blobs(testbloblist); */
/*   printf("Sorted_blobs: \n"); */
/*   bloblist_print(testbloblist); */
/*   printf("deleteme test end\n"); */
/*   exit(1); */
/*   /\* DELETEME TEST END *\/ */

/*   printf("Sorted_blobs: \n"); */
/*   bloblist_print(blobs); */

//  print_matrix(model_base, "Model_base");
//  print_vec(model_ref, "Ref_point");
  alter_pose(blobs, points, centering);
//  print_matrix(points, "Alter");

  double new_base[3][3];
  double vec1[3];
  double vec2[3];
  ltr_int_transpose_in_place(points);
//  print_matrix(points, "Alter");
  ltr_int_make_vec(points[1], points[0], vec1);
  ltr_int_make_vec(points[2], points[0], vec2);
//  print_vec(vec1, "vec1");
//  print_vec(vec2, "vec2");
  ltr_int_make_base(vec1, vec2, new_base);
  if(centering == true){
    /*log_message("Center:\n");
    log_message("[%g, %g]\n", blobs.blobs[0].x, blobs.blobs[0].y);
    log_message("[%g, %g]\n", blobs.blobs[1].x, blobs.blobs[1].y);
    log_message("[%g, %g]\n", blobs.blobs[2].x, blobs.blobs[2].y);*/
/*     print_matrix(center_base, "center-base"); */
/*     print_matrix(new_base, "new-base"); */
    int i,j;
    for(i = 0; i < 3; ++i){
      for(j = 0; j < 3; ++j){
        center_base[i][j] = new_base[i][j];
      }
    }
  }
//  print_matrix(new_base, "New_base");
//  double new_ref[3];
  get_translation(new_base, model_ref, points[0], trans->tr, centering);
//  print_vec(trans->tr, "translation");
  get_transform(new_base, trans->rot);
//  print_matrix(trans->rot, "Rot");
  return true;
}

/*
static void transform_print(struct transform trans)
{
  double ypr[3]; // yaw, pitch, roll;
  printf("***** Transform **************\n");
  ltr_int_print_vec(trans.tr, "translation");
  ltr_int_print_matrix(trans.rot, "rotation");
  ltr_int_matrix_to_euler(trans.rot, &(ypr[1]), &(ypr[0]), &(ypr[2]));
  ypr[0] *= 180.0/M_PI;
  ypr[1] *= 180.0/M_PI;
  ypr[2] *= 180.0/M_PI;
  ltr_int_print_vec(ypr, "angles");
  printf("******************************\n");
}
*/

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
    /* convert to degrees */
    (*pitch) = p * 180.0 /M_PI;
    (*yaw)   = y * 180.0 /M_PI;
    (*roll)  = r * 180.0 /M_PI;
    #ifdef PT_DBG
      printf("ROT: %g %g %g\n", *pitch, *yaw, *roll);
      printf("TRA: %g %g %g\n", *tx, *ty, *tz);
    #endif
  
    return 0;
  }
  return -1;
}

/*
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
