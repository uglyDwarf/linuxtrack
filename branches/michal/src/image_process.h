#ifndef IMAGE_PROCESS__H
#define IMAGE_PROCESS__H

#include "cal.h"
#include "list.h"

typedef struct {
  unsigned int vline;
  unsigned int hstart;
  unsigned int hstop;
  unsigned int sum_x;
  unsigned int sum;
  unsigned int points;
} stripe_t;

typedef struct {
  int w, h;
  unsigned char *bitmap;
  float ratio;
} image;

void ltr_int_prepare_for_processing(int w, int h);
void ltr_int_to_stripes(image *img);
int ltr_int_stripes_to_blobs(int num_blobs, struct bloblist_type *blt, 
		     int min_pts, int max_pts, image *img);
bool ltr_int_add_stripe(stripe_t *stripe, image *img);
void ltr_int_draw_cross(image *img, int x, int y, int size);
void ltr_int_draw_square(image *img, int x, int y, int size);

#endif
