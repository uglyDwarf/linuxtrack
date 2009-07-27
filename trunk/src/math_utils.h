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

void make_vec(float pt1[3],float pt2[3],float res[3]);
float vec_size(float vec[3]);
float dot_product(float vec1[3],float vec2[3]);
void cross_product(float vec1[3],float vec2[3],float res[3]);
void mul_matrix(float m1[3][3], float m2[3][3], float res[3][3]);
void mul_vec(float vec[3],float c,float res[3]);
void matrix_times_vec(float m[3][3], float vec[3],float res[3]);
void make_base(float vec1[3],float vec2[3],float res[3][3]);
void print_matrix(float matrix[3][3], char *name);
void print_vec(float vec[3], char *name);
void transpose(float matrix[3][3], float trans[3][3]);
void transpose_in_place(float matrix[3][3]);
float sqr(float f);
void matrix_to_euler(float matrix[3][3], float *pitch, float *yaw, float *roll);
void add_vecs(float vec1[3],float vec2[3],float res[3]);
bool make_bez(float deadzone, float k, bez_def *b);
float bezier(float x, bez_def *b);

#endif
