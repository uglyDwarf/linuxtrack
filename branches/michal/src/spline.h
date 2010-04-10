#ifndef SPLINE__H
#define SPLINE__H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
  float dead_zone;
  float l_curvature;
  float r_curvature;
}splines_def;

typedef struct{
  float x0, y0;
  float x1, y1;
  float x2, y2;
}spline_pts;

typedef struct{
  spline_pts left, right;
}splines;

//Converts curve definition to spline points
int curve2pts(const splines_def *curve, splines *pts);

//Make sure x belongs to <-1; 1>!
float spline_point(const splines *pts, float x);

#ifdef __cplusplus
}
#endif

#endif
