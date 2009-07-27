
#include "math_utils.h"
#include <stdio.h>
void make_vec(float pt1[3],float pt2[3],float res[3])
{
  res[0]=pt1[0]-pt2[0];
  res[1]=pt1[1]-pt2[1];
  res[2]=pt1[2]-pt2[2];
}

float vec_size(float vec[3])
{
  return(sqrt(vec[0]*vec[0]+vec[1]*vec[1]+vec[2]*vec[2]));
}

float dot_product(float vec1[3],float vec2[3])
{
  return(vec1[0]*vec2[0]+vec1[1]*vec2[1]+vec1[2]*vec2[2]);
}

void cross_product(float vec1[3],float vec2[3],float res[3])
{
  res[0]=vec1[1]*vec2[2]-vec1[2]*vec2[1];
  res[1]=vec1[2]*vec2[0]-vec1[0]*vec2[2];
  res[2]=vec1[0]*vec2[1]-vec1[1]*vec2[0];
}

void mul_vec(float vec[3],float c,float res[3])
{
  res[0]=vec[0]*c;
  res[1]=vec[1]*c;
  res[2]=vec[2]*c;
}

void make_base(float vec1[3],float vec2[3],float res[3][3])
{
  float tmp1[3],tmp2[3];
  mul_vec(vec1,1/vec_size(vec1),res[0]);
  mul_vec(res[0],dot_product(vec2,res[0]),tmp1);
  make_vec(vec2,tmp1,tmp2);
  mul_vec(tmp2,1/vec_size(tmp2),res[1]);
  cross_product(res[0],res[1],res[2]);
  
}

void matrix_times_vec(float m[3][3], float vec[3],float res[3])
{
  res[0] = dot_product(m[0], vec);
  res[1] = dot_product(m[1], vec);
  res[2] = dot_product(m[2], vec);
}

void print_matrix(float matrix[3][3], char *name)
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

void print_vec(float vec[3], char *name)
{
/*   printf("%s = [%15f; %15f; %15f]\n", name, vec[0], vec[1], vec[2]); */
  printf("%s=[%f;%f;%f]\n", name, vec[0], vec[1], vec[2]);
}

float sqr(float f)
{
  return(f*f);
}

void mul_matrix(float m1[3][3], float m2[3][3], float res[3][3])
{
  int i,j;
  float m2t[3][3];
  transpose(m2, m2t);
  for(i = 0; i < 3; ++i){
    for(j = 0; j < 3; ++j){
      res[i][j] = dot_product(m1[i], m2t[j]);
    }
  }
}

void transpose(float matrix[3][3], float trans[3][3])
{
  int i,j;
  for(i = 0; i < 3; ++i){
    for(j = 0; j < 3; ++j){
      trans[i][j] = matrix[j][i];
    }
  }
}

void swp(float *f1, float *f2)
{
  float tmp;
  tmp = *f1;
  *f1 = *f2;
  *f2 = tmp;
}

void transpose_in_place(float matrix[3][3])
{
  swp(&matrix[0][1], &matrix[1][0]);
  swp(&matrix[0][2], &matrix[2][0]);
  swp(&matrix[1][2], &matrix[2][1]);
}

void matrix_to_euler(float matrix[3][3], float *pitch, float *yaw, float *roll)
{
  float tmp = matrix[0][2];
  if(tmp < -1.0f) 
    tmp = -1.0f;
  if(tmp > 1.0f)
    tmp = 1.0f;
  *yaw = asinf(tmp);
  float yc = cosf(*yaw);
  if (fabs(yc) > 1e-5){
    *pitch = atan2f(matrix[1][2]/yc, matrix[2][2]/yc);
    *roll = atan2f(-matrix[0][1]/yc, matrix[0][0]/yc);
  }else{
    *pitch = 0.0f;
    *roll = atan2f(matrix[1][0], matrix[1][1]);
  }
}

void euler_to_matrix(float pitch, float yaw, float roll, float matrix[3][3])
{
  float cp = cosf(-pitch);
  float sp = sinf(-pitch);
  float cy = cosf(-yaw);
  float sy = sinf(-yaw);
  float cr = cosf(-roll);
  float sr = sinf(-roll);
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


void add_vecs(float vec1[3],float vec2[3],float res[3])
{
  res[0] = vec1[0] + vec2[0];
  res[1] = vec1[1] + vec2[1];
  res[2] = vec1[2] + vec2[2];
}


bool make_bez(float deadzone, float k, bez_def *b)
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

float bezier(float x, bez_def *b)
{
  if(x < b->p0_x){
    return 0.0f;
  }
  float c = b->cx - x;
  float t = (-(b->bx) + sqrt((b->bx * b->bx) - 4 * b->ax * c)) / (2 * b->ax);
  float y = (t * t) * b->ay + t * b->by + b->cy;
  return y;
}

/*
Bezier usage example
  bez_def b;
  make_bez(0.0f, 0.6, &b);
  y = bezier(x, &b);
*/

bool is_finite(float f)
{
  if(finite(f) != 0){
    return true;
  }else{
    return false;
  }
}


