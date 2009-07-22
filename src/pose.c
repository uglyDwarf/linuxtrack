#include <stdio.h>
#include "pose.h"
#include "math_utils.h"
#include "cal.h"

/* Focal length */
float internal_focal_depth;

float model_point0[3];
float model_point1[3];
float model_point2[3];

float model_ref[3];

float model_base[3][3];

float center_ref[3];
float center_base[3][3];

enum {M_CAP, M_CLIP} type;

volatile bool center_flag = false;

void pose_init(struct reflector_model_type rm,
               float focal_depth)
{
  internal_focal_depth = focal_depth;
  /* Physical dimensions */
  /* Camera looks in direction of Z axis */
  /* X axis goes then to the right */
  /* Y axis goes up */
/*   float x = 168; */
/*   float y = 90; */
/*   float z = 100; */

/*   /\* Point, around which model rotates (just dimensions)*\/ */
/*   float ref_y = 150; */
/*   float ref_z = 150; */

/*   model_point0[0] = 0.0;  */
/*   model_point0[1] = y;  */
/*   model_point0[2] = z; */
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
  float ref[3];
  ref[0] = rm.hc[0];
  ref[1] = rm.hc[1];
  ref[2] = rm.hc[2];


  /* Out of model points create orthonormal base */
  float vec1[3];
  float vec2[3];
  make_vec(model_point1, model_point0, vec1);
  make_vec(model_point2, model_point0, vec2);
  make_base(vec1, vec2, model_base);

//for testing purposes
  make_base(vec1, vec2, center_base);

  /* Convert reference point to model base coordinates */
//  float ref_pt[3];
  float vec3[3];
  make_vec(ref, model_point0, vec3);
  matrix_times_vec(model_base, vec3, model_ref);
  switch(rm.type){
    case CAP:
      type = M_CAP;
      break;
    case CLIP:
      type = M_CLIP;
      break;
    default:
      break;
  }
}

float blob_dist(struct blob_type b0, struct blob_type b1)
{
  return sqrt(sqr(b1.x-b0.x) + sqr(b1.y-b0.y));
}

void alter_pose(struct bloblist_type blobs, float points[3][3])
{
  float R01, R02, R12, d01, d02, d12, a, b, c, s, h1, h2, sigma;
  float tmp[3];

  make_vec(model_point0, model_point1, tmp);
  R01 = vec_size(tmp);
  make_vec(model_point0, model_point2, tmp);
  R02 = vec_size(tmp);
  make_vec(model_point1, model_point2, tmp);
  R12 = vec_size(tmp);

  d01 = blob_dist(blobs.blobs[0], blobs.blobs[1]);
  d02 = blob_dist(blobs.blobs[0], blobs.blobs[2]);
  d12 = blob_dist(blobs.blobs[1], blobs.blobs[2]);

  a = (R01 + R02+ R12) * (-R01 + R02 + R12)
    * (R01 - R02 + R12) * (R01 + R02 - R12);

  b = sqr(d01) * (-sqr(R01) + sqr(R02) + sqr(R12))
    + sqr(d02) * (sqr(R01) - sqr(R02) + sqr(R12))
    + sqr(d12) * (sqr(R01) + sqr(R02) - sqr(R12));

  c = (d01 + d02+ d12) * (-d01 + d02 + d12)
    * (d01 - d02 + d12) * (d01 + d02 - d12);

  s = sqrt((b + sqrt(sqr(b) - a * c)) / a);

  if((sqr(d01) + sqr(d02) - sqr(d12))
      <= (sqr(s) * (sqr(R01) + sqr(R02) - sqr(R12)))){
    sigma = 1;
  }else{
    sigma = -1;
  }

  h1 = -sqrt(sqr(s * R01) - sqr(d01));
  h2 = -sigma * sqrt(sqr(s * R02) - sqr(d02));

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
}

void get_translation(float base[3][3], float ref[3], float origin[3],
                     float trans[3], bool do_center){
//  float tmp[3];
  float t_base[3][3];
  float new_ref[3];
  transpose(base, t_base);
  matrix_times_vec(t_base, ref, new_ref);
  new_ref[0] += origin[0];
  new_ref[1] += origin[1];
  new_ref[2] += origin[2];
//  print_vec(new_ref, "new_ref");
//  print_vec(origin, "origin");
  if(do_center == true){
    center_ref[0] = new_ref[0];
    center_ref[1] = new_ref[1];
    center_ref[2] = new_ref[2];
  }
  trans[0] = new_ref[0] - center_ref[0];
  trans[1] = new_ref[1] - center_ref[1];
  trans[2] = new_ref[2] - center_ref[2];
}

void get_transform(float new_base[3][3], float rot[3][3]){
  float center_base_t[3][3];
  transpose(center_base, center_base_t);
  mul_matrix(center_base_t, new_base, rot);
}

void pose_sort_blobs(struct bloblist_type bl)
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

bool pose_process_blobs(struct bloblist_type blobs,
                        struct transform *trans)
{

  float points[3][3];
  bool centering = false;

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
  alter_pose(blobs, points);
//  print_matrix(points, "Alter");

  float new_base[3][3];
  float vec1[3];
  float vec2[3];
  transpose_in_place(points);
//  print_matrix(points, "Alter");
  make_vec(points[1], points[0], vec1);
  make_vec(points[2], points[0], vec2);
//  print_vec(vec1, "vec1");
//  print_vec(vec2, "vec2");
  make_base(vec1, vec2, new_base);
  if(center_flag == true){
    center_flag = false;
    centering = true;
    int i,j;
    for(i = 0; i < 3; ++i){
      for(j = 0; j < 3; ++j){
        center_base[i][j] = new_base[i][j];
      }
    }
  }
//  print_matrix(new_base, "New_base");
//  float new_ref[3];
  get_translation(new_base, model_ref, points[0], trans->tr, centering);
//  print_vec(trans->tr, "translation");
  get_transform(new_base, trans->rot);
//  print_matrix(trans->rot, "Rot");
  center_flag = false;
  return true;
}

void transform_print(struct transform trans)
{
  float ypr[3]; /* yaw, pitch, roll;*/
  printf("***** Transform **************\n");
  print_vec(trans.tr, "translation");
  print_matrix(trans.rot, "rotation");
  matrix_to_euler(trans.rot, &(ypr[1]), &(ypr[0]), &(ypr[2]));
  ypr[0] *= 180.0/M_PI;
  ypr[1] *= 180.0/M_PI;
  ypr[2] *= 180.0/M_PI;
  print_vec(ypr, "angles");
  printf("******************************\n");
}

int pose_compute_camera_update(struct transform trans,
                               float *yaw,
                               float *pitch,
                               float *roll,
                               float *tx,
                               float *ty,
                               float *tz)
{
  *tx = trans.tr[0];
  *ty = trans.tr[1];
  *tz = trans.tr[2];
  matrix_to_euler(trans.rot, pitch, yaw, roll);
  /* convert to degrees */
  (*pitch) *= 180.0/M_PI;
  (*yaw)   *= 180.0/M_PI;
  (*roll)  *= 180.0/M_PI;
  return 0;
}


void pose_recenter(void)
{
  center_flag = true;
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

  float pitch, roll, yaw;
  matrix_to_euler(res.rot, &pitch, &yaw, &roll);
  printf("Pitch: %f  Yaw: %f  Roll: %f\n", pitch, yaw, roll);

  return 0;
}
*/
