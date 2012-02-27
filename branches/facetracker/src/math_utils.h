#ifndef MATH_UTILS__H
#define MATH_UTILS__H

#include <stdbool.h>
#include <math.h>

typedef struct bez_def {
  double p0_x, p0_y;
  double p1_x, p1_y;
  double p2_x, p2_y;
  double ax, bx, cx;
  double ay, by, cy;
} bez_def;

void ltr_int_make_vec(double pt1[3],double pt2[3],double res[3]);
double ltr_int_vec_size(double vec[3]);
double ltr_int_dot_product(double vec1[3],double vec2[3]);
void ltr_int_cross_product(double vec1[3],double vec2[3],double res[3]);
void ltr_int_assign_matrix(double src[3][3], double tgt[3][3]);
void ltr_int_mul_matrix(double m1[3][3], double m2[3][3], double res[3][3]);
void ltr_int_mul_vec(double vec[3],double c,double res[3]);
void ltr_int_normalize_vec(double vec[3]);
void ltr_int_matrix_times_vec(double m[3][3], double vec[3],double res[3]);
void ltr_int_make_base(double vec1[3],double vec2[3],double res[3][3]);
void ltr_int_print_matrix(double matrix[3][3], char *name);
void ltr_int_print_vec(double vec[3], char *name);
void ltr_int_transpose(double matrix[3][3], double trans[3][3]);
void ltr_int_transpose_in_place(double matrix[3][3]);
double ltr_int_sqr(double f);
void ltr_int_matrix_to_euler(double matrix[3][3], double *pitch, double *yaw, double *roll);
void ltr_int_euler_to_matrix(double pitch, double yaw, double roll, double matrix[3][3]);
void ltr_int_add_vecs(double vec1[3],double vec2[3],double res[3]);
bool ltr_int_make_bez(double deadzone, double k, bez_def *b);
double ltr_int_bezier(double x, bez_def *b);
bool ltr_int_is_finite(double f);
double clamp_angle(double angle);
#endif
