
#include "math_utils.h"
#include <stdio.h>
void ltr_int_make_vec(double pt1[3],double pt2[3],double res[3])
{
  res[0]=pt1[0]-pt2[0];
  res[1]=pt1[1]-pt2[1];
  res[2]=pt1[2]-pt2[2];
}

double ltr_int_vec_size(double vec[3])
{
  return(sqrt(vec[0]*vec[0]+vec[1]*vec[1]+vec[2]*vec[2]));
}

double ltr_int_dot_product(double vec1[3],double vec2[3])
{
  return(vec1[0]*vec2[0]+vec1[1]*vec2[1]+vec1[2]*vec2[2]);
}

void ltr_int_cross_product(double vec1[3],double vec2[3],double res[3])
{
  res[0]=vec1[1]*vec2[2]-vec1[2]*vec2[1];
  res[1]=vec1[2]*vec2[0]-vec1[0]*vec2[2];
  res[2]=vec1[0]*vec2[1]-vec1[1]*vec2[0];
}

void ltr_int_mul_vec(double vec[3],double c,double res[3])
{
  res[0]=vec[0]*c;
  res[1]=vec[1]*c;
  res[2]=vec[2]*c;
}

void ltr_int_make_base(double vec1[3],double vec2[3],double res[3][3])
{
  double tmp1[3],tmp2[3];
  ltr_int_mul_vec(vec1,1/ltr_int_vec_size(vec1),res[0]);
  ltr_int_mul_vec(res[0],ltr_int_dot_product(vec2,res[0]),tmp1);
  ltr_int_make_vec(vec2,tmp1,tmp2);
  ltr_int_mul_vec(tmp2,1/ltr_int_vec_size(tmp2),res[1]);
  ltr_int_cross_product(res[0],res[1],res[2]);
  
}

void ltr_int_matrix_times_vec(double m[3][3], double vec[3],double res[3])
{
  res[0] = ltr_int_dot_product(m[0], vec);
  res[1] = ltr_int_dot_product(m[1], vec);
  res[2] = ltr_int_dot_product(m[2], vec);
}

void ltr_int_print_matrix(double matrix[3][3], char *name)
{
  int i,j;
/*   printf("\n%s = [\n", name); */
  printf("%s=[", name);
  for(i = 0; i < 3; ++i){
    for(j = 0; j < 3; ++j){
/*       printf("%15f,", matrix[i][j]); */
      printf("%f,", matrix[i][j]);
    }
/*     printf("\n"); */
    printf(";");
  }
  printf("]\n");
}

void ltr_int_print_vec(double vec[3], char *name)
{
/*   printf("%s = [%15f; %15f; %15f]\n", name, vec[0], vec[1], vec[2]); */
  printf("%s=[%f;%f;%f]\n", name, vec[0], vec[1], vec[2]);
}

double ltr_int_sqr(double f)
{
  return(f*f);
}

void ltr_int_mul_matrix(double m1[3][3], double m2[3][3], double res[3][3])
{
  int i,j;
  double m2t[3][3];
  ltr_int_transpose(m2, m2t);
  for(i = 0; i < 3; ++i){
    for(j = 0; j < 3; ++j){
      res[i][j] = ltr_int_dot_product(m1[i], m2t[j]);
    }
  }
}

void ltr_int_transpose(double matrix[3][3], double trans[3][3])
{
  int i,j;
  for(i = 0; i < 3; ++i){
    for(j = 0; j < 3; ++j){
      trans[i][j] = matrix[j][i];
    }
  }
}

static void swp(double *f1, double *f2)
{
  double tmp;
  tmp = *f1;
  *f1 = *f2;
  *f2 = tmp;
}

void ltr_int_transpose_in_place(double matrix[3][3])
{
  swp(&matrix[0][1], &matrix[1][0]);
  swp(&matrix[0][2], &matrix[2][0]);
  swp(&matrix[1][2], &matrix[2][1]);
}

void ltr_int_matrix_to_euler(double matrix[3][3], double *pitch, double *yaw, double *roll)
{
  double tmp = matrix[0][2];
  if(tmp < -1.0f) 
    tmp = -1.0f;
  if(tmp > 1.0f)
    tmp = 1.0f;
  *yaw = asin(tmp);
  double yc = cos(*yaw);
  if (fabs(yc) > 1e-5){
    *pitch = atan2(matrix[1][2]/yc, matrix[2][2]/yc);
    *roll = atan2(-matrix[0][1]/yc, matrix[0][0]/yc);
  }else{
    *pitch = 0.0f;
    *roll = atan2(matrix[1][0], matrix[1][1]);
  }
}

void ltr_int_euler_to_matrix(double pitch, double yaw, double roll, double matrix[3][3])
{
  double cp = cos(-pitch);
  double sp = sin(-pitch);
  double cy = cos(-yaw);
  double sy = sin(-yaw);
  double cr = cos(-roll);
  double sr = sin(-roll);
  matrix[0][0] = cr * cy;
  matrix[0][1] = sr * cy;
  matrix[0][2] = -sy;

  matrix[1][0] = cr * sy * sp - sr * cp;
  matrix[1][1] = sr * sy * sp + cr * cp;
  matrix[1][2] = cy * sp;

  matrix[2][0] = cr * sy * cp + sr * sp;
  matrix[2][1] = sr * sy * cp - cr * sp;
  matrix[2][2] = cy * cp;
}


void ltr_int_add_vecs(double vec1[3],double vec2[3],double res[3])
{
  res[0] = vec1[0] + vec2[0];
  res[1] = vec1[1] + vec2[1];
  res[2] = vec1[2] + vec2[2];
}


bool ltr_int_make_bez(double deadzone, double k, bez_def *b)
{
  b->p0_x = deadzone;
  b->p0_y = 0.0f;
  b->p1_x = k;
  b->p1_y = 1.0 - k;
  b->p2_x = 1.0f;
  b->p2_y = 1.0f;

  b->ax = b->p0_x - 2 * b->p1_x + b->p2_x;
  b->bx = 2 * (b->p1_x - b->p0_x);
  b->cx = b->p0_x;
  
  b->ay = b->p0_y - 2 * b->p1_y + b->p2_y;
  b->by = 2 * (b->p1_y - b->p0_y);
  b->cy = b->p0_y;
  return true;
}

double ltr_int_bezier(double x, bez_def *b)
{
  if(x < b->p0_x){
    return 0.0f;
  }
  double c = b->cx - x;
  double t = (-(b->bx) + sqrt((b->bx * b->bx) - 4 * b->ax * c)) / (2 * b->ax);
  double y = (t * t) * b->ay + t * b->by + b->cy;
  return y;
}

/*
Bezier usage example
  bez_def b;
  make_bez(0.0f, 0.6, &b);
  y = bezier(x, &b);
*/

bool ltr_int_is_finite(double f)
{
  if(finite(f) != 0){
    return true;
  }else{
    return false;
  }
}


