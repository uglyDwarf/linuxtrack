#ifndef AXIS__H
#define AXIS__H

#ifdef __cplusplus
extern "C" {
#endif

#include "spline.h"

typedef struct{
  splines_def curves;
  float l_factor, r_factor;
} axis_def;


#ifdef __cplusplus
}
#endif

#endif
