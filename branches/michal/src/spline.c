#include <math.h>
#include "spline.h"




int curve2pts(const splines_def *curve, splines *pts)
{
  pts->left.x0 = pts->right.x0 = curve->dead_zone;
  pts->left.y0 = pts->right.y0 = 0.0f;
  
  pts->left.x1 = curve->dead_zone + (1.0f - curve->dead_zone) * (1.0f - curve->l_curvature);
  pts->right.x1 = curve->dead_zone + (1.0f - curve->dead_zone) * (1.0f - curve->r_curvature);
  pts->left.y1 = curve->l_curvature;
  pts->right.y1 = curve->r_curvature;

  pts->left.x2 = pts->right.x2 = 1.0f;
  pts->left.y2 = pts->right.y2 = 1.0f;
  return 0;
}

float spline_point(const splines *splns, float x)
{
  const spline_pts *pts;
  float a, b, c;
  float factor;
  if(x < 0.0f){
    pts = &(splns->left);
    factor = -1.0f;
    x *= -1.0f;
  }else{
    pts = &(splns->right);
    factor = 1.0f;
  }
  a = pts->x0;
  if(x < a){
    return 0.0f;
  }
  b = pts->x1;
  c = pts->x2;
  float tmp = c - 2 * b + a;
  if(fabs(tmp) < 1e-3){ //simple linear
    return factor * (x - a) * ((pts->y2 - pts->y0) / (c - a));
  }else{
    float t = (sqrtf(tmp * x - a * c + b * b) - b + a) / tmp;
    float b02 = (1 - t) * (1 - t);
    float b12 = 2 * t * (1 - t);
    float b22 = t * t;
    return factor * ((b02 * pts->y0) + (b12 * pts->y1) + (b22 * pts->y2));
  }
}

