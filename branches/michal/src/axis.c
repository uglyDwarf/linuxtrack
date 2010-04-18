#include "axis.h"

float val_on_axis(axis_def *axis, float x)
{
  if(!(axis->valid)){
    curve2pts(&(axis->curve_defs), &(axis->curves));
  }
  x = x / axis->limits;
  if(x < -1.0){
    x = -1.0;
  }
  if(x > 1.0){
    x = 1.0;
  }
  float y = spline_point(&(axis->curves), x);
  if(x < 0.0){
    return y * axis->l_factor; 
  }else{
    return y * axis->r_factor; 
  }
}

bool is_symetrical(const axis_def *axis)
{
  if((axis->l_factor == axis->r_factor) && 
     (axis->curve_defs.l_curvature == axis->curve_defs.l_curvature)){
    return true;
  }else{
    return false;
  }
}

bool set_deadzone(axis_def *axis, float dz)
{
  axis->valid = false;
  axis->curve_defs.dead_zone = dz;
  
  return true;
}

bool set_lcurv(axis_def *axis, float c)
{
  axis->valid = false;
  axis->curve_defs.l_curvature = c;
  
  return true;
}

bool set_rcurv(axis_def *axis, float c)
{
  axis->valid = false;
  axis->curve_defs.r_curvature = c;
  
  return true;
}

bool set_lmult(axis_def *axis, float m1)
{
  axis->valid = false;
  axis->l_factor = m1;
  
  return true;
}

bool set_rmult(axis_def *axis, float m1)
{
  axis->valid = false;
  axis->r_factor = m1;
  
  return true;
}

bool set_limits(axis_def *axis, float lim)
{
  axis->valid = false;
  axis->limits = lim;
  
  return true;
}

