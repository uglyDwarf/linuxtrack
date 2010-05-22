#ifndef MATH_UTILS__H
#define MATH_UTILS__H

#include <stdbool.h>
#include <math.h>

typedef struct bez_def {
  float p0_x, p0_y;
  float p1_x, p1_y;
  float p2_x, p2_y;
  float ax, bx, cx;
  float ay, by, cy;
} bez_def;

void ltr_int_make_vec(float pt1[3],float pt2[3],float res[3]);
float ltr_int_vec_size(float vec[3]);
float ltr_int_dot_product(float vec1[3],float vec2[3]);
void ltr_int_cross_product(float vec1[3],float vec2[3],float res[3]);
void ltr_int_mul_matrix(float m1[3][3], float m2[3][3], float res[3][3]);
void ltr_int_mul_vec(float vec[3],float c,float res[3]);
void ltr_int_matrix_times_vec(float m[3][3], float vec[3],float res[3]);
void ltr_int_make_base(float vec1[3],float vec2[3],float res[3][3]);
void ltr_int_print_matrix(float matrix[3][3], char *name);
void ltr_int_print_vec(float vec[3], char *name);
void ltr_int_transpose(float matrix[3][3], float trans[3][3]);
void ltr_int_transpose_in_place(float matrix[3][3]);
float ltr_int_sqr(float f);
void ltr_int_matrix_to_euler(float matrix[3][3], float *pitch, float *yaw, float *roll);
void ltr_int_euler_to_matrix(float pitch, float yaw, float roll, float matrix[3][3]);
void ltr_int_add_vecs(float vec1[3],float vec2[3],float res[3]);
bool ltr_int_make_bez(float deadzone, float k, bez_def *b);
float ltr_int_bezier(float x, bez_def *b);
bool ltr_int_is_finite(float f);

#endif
