#include <stdio.h>
#include "pose.h"
#include "math_utils.h"
#include "cal.h"

/* Focal length */
float focal_length = 10;

float model_point0[3];
float model_point1[3];
float model_point2[3];

float model_ref[3];

float model_base[3][3];

float center_ref[3];
float center_base[3][3];

int center_flag = 0;

void init_model()
{
  /* Physical dimensions */
  /* Camera looks in direction of Z axis */
  /* X axis goes then to the left */
  /* Y axis goes up */
  float x = 168;
  float y = 90;
  float z = 100;
  
  /* Point, around which model rotates (just dimensions)*/
  float ref_y = 150;
  float ref_z = 150;
  float ref[3];
  
  model_point0[0] = 0.0; 
  model_point0[1] = y; 
  model_point0[2] = z;
  
  model_point1[0] = -x/2; 
  model_point1[1] = 0.0; 
  model_point1[2] = 0.0;
  
  model_point2[0] = x/2; 
  model_point2[1] = 0.0; 
  model_point2[2] = 0.0;
  
  ref[0] = 0.0; 
  ref[1] = -ref_y; 
  ref[2] = ref_z;
  
  /* Out of model points create orthonormal base */
  float vec1[3];
  float vec2[3];
  make_vec(model_point1, model_point0, vec1);
  make_vec(model_point2, model_point0, vec2);
  make_base(vec1, vec2, model_base);

//for testing purposes
  make_base(vec1, vec2, center_base);
  
  /* Convert reference point to model base coordinates */
  float ref_pt[3];
  float vec3[3];
  make_vec(ref, model_point0, vec3);
  matrix_times_vec(model_base, vec3, model_ref);
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
  points[2][0] = focal_length / s;
  
  points[0][1] = blobs.blobs[1].x / s;
  points[1][1] = blobs.blobs[1].y / s;
  points[2][1] = (focal_length + h1) / s;
  
  points[0][2] = blobs.blobs[2].x / s;
  points[1][2] = blobs.blobs[2].y / s;
  points[2][2] = (focal_length + h2) / s;
  
}

void get_translation(float base[3][3], float ref[3], float origin[3], 
                                                        float trans[3]){
  float tmp[3];
  float t_base[3][3];
  float new_ref[3];
  transpose(base, t_base);
  matrix_times_vec(t_base, ref, new_ref);
  new_ref[0] += origin[0];
  new_ref[1] += origin[1];
  new_ref[2] += origin[2];
//  print_vec(new_ref, "new_ref");
//  print_vec(origin, "origin");
  trans[0] = new_ref[0] - center_ref[0];
  trans[1] = new_ref[1] - center_ref[1];
  trans[2] = new_ref[2] - center_ref[2];
}

void get_transform(float new_base[3][3], float rot[3][3]){
  float center_base_t[3][3], new_base_t[3][3];
  transpose(center_base, center_base_t);   
  mul_matrix(center_base_t, new_base, rot);
}

bool process_blobs(struct bloblist_type blobs, struct transform *trans)
{
  
  float points[3][3];
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
//  print_matrix(new_base, "New_base");
  float new_ref[3];
  get_translation(new_base, model_ref, points[0], trans->tr);
//  print_vec(trans->tr, "translation");
  get_transform(new_base, trans->rot);
//  print_matrix(trans->rot, "Rot");
  return true;
}

void print_transform(struct transform trans)
{
  printf("******************************\ntranslation:\n");
  printf("translation:\n");
  printf("x: %f\ty: %f\tz: %f\n", trans.tr[0], trans.tr[1], trans.tr[2]);
  printf("rotation:\n");
  printf("m00: %f\tm01: %f\tm02: %f\n", 
         trans.rot[0][0], trans.rot[0][1], trans.rot[0][2]);
  printf("m10: %f\tm11: %f\tm12: %f\n", 
         trans.rot[1][0], trans.rot[1][1], trans.rot[1][2]);
  printf("m20: 2f\tm21: %f\tm22: %f\n", 
         trans.rot[2][0], trans.rot[2][1], trans.rot[2][2]);
  printf("******************************\n");
}

/*
int main(void)
{
  init_model();
  
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
  process_blobs(blobs ,&res);
  
  float pitch, roll, yaw;
  matrix_to_euler(res.rot, &pitch, &yaw, &roll);
  printf("Pitch: %f  Yaw: %f  Roll: %f\n", pitch, yaw, roll);
  
  return 0;
}
*/
