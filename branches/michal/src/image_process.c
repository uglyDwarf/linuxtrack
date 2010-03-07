#include <stdbool.h>
#include <assert.h>
#include "image_process.h"
#include "list.h"
#include "utils.h"


typedef struct preblob_t{
  float sum_x, sum_y; //sums of pixval and coord products
  int sum; //sum of pixel weights 
  int points; //pixel count
  bool added;
  bool matched;
} preblob_t;

static plist preblobs = NULL;


typedef struct {
  unsigned int x1,x2;
  preblob_t *pb;
} range;

typedef struct {
  range *ranges;
  int limit;
} stripe_array;

static stripe_array current = {
  .ranges = NULL,
  .limit = 0,
};

static stripe_array next = {
  .ranges = NULL,
  .limit = 0,
};

static int current_vline = -2;


static void clip_coord(int *coord, int min,
                       int max)
{
  int tmp = *coord;
  tmp = (tmp < min) ? min : tmp;
  tmp = (tmp > max) ? max : tmp;
  *coord = tmp;
}

static void draw_stripe(image *img, int x, int y, int x_end, unsigned char color)
{
  assert(img != NULL);
  y *= img->ratio;
  clip_coord(&x, 0, img->w);
  clip_coord(&y, 0, img->h);
  clip_coord(&x_end, 0, img->w);
  unsigned char *ptr = img->bitmap + y * img->w + x;
  x_end -= x;
  while(x_end > 0){
    *(++ptr) = color;
    --x_end;
  }
}


void draw_square(image *img, int x, int y, int size)
{
  assert(img != NULL);
  assert(x >= 0);
  assert(y >= 0);
  int x1 = (int)x - size;
  int x2 = (int)x + size;
  int y1 = (y * img->ratio) - size;
  int y2 = (y * img->ratio) + size;
  
//  clip_coord(&x, 0, img->w);
  clip_coord(&x1, 0, img->w);
  clip_coord(&y1, 0, img->h);
  clip_coord(&x2, 0, img->w);
  clip_coord(&y2, 0, img->h);
  
  while(y1 < y2){
    draw_stripe(img, x1, y1, x2, 0xFF);
    ++y1;
  }
}


void draw_cross(image *img, int x, int y, int size)
{
  int cntr;
  int x_m = x - size;
  int x_p = x + size;
  int y_m = y - size;
  int y_p = y + size;
  clip_coord(&x_m, 0, img->w);
  clip_coord(&x_p, 0, img->w);
  clip_coord(&y_m, 0, img->h);
  clip_coord(&y_p, 0, img->h);
  
  unsigned char *pt;
  pt = img->bitmap + (img->w * y) + x_m;
  for(cntr = x_m; cntr < x_p; ++cntr){
    *(pt++) = 0xFF;
  }
  pt = img->bitmap + (img->w * y_m) + x;
  for(cntr = y_m; cntr < y_p; ++cntr){
    *pt = 0xFF;
    pt += img->w;
  }
}




struct blob_type* new_blob(float x, float y, int score)
{
  struct blob_type *tmp = (struct blob_type*)my_malloc(sizeof(struct blob_type));
  tmp->x = x;
  tmp->y = y;
  tmp->score = score;
  return tmp;
}

static bool stripe_in_range(stripe_t *stripe, range *rng)
{
#ifdef DBG_MSG
  printf("Testing coincidence!\n");
  printf("Stripe: y:%d   x:%d - %d (%d   %d)\n", stripe->vline, stripe->hstart, 
         stripe->hstop, stripe->sum, stripe->sum_x);
  printf("Range: x:%d - %d\n", rng->x1, rng->x2);
#endif
  if((((int)stripe->hstart)-1) <= ((int)rng->x2)){
    if((stripe->hstop+1) >= rng->x1){
#ifdef DBG_MSG
      printf("Coincide!\n");
#endif
      return true;
    }
  }
  return false;
}

static void merge_preblobs(preblob_t *b1, preblob_t *b2)
{
#ifdef DBG_MSG
  printf("Merging %p and %p\n", b1, b2);
#endif
  assert(b1 != NULL);
  assert(b2 != NULL);
  b1->sum_x += b2->sum_x;
  b1->sum_y += b2->sum_y;
  b1->sum += b2->sum;
  b1->points += b2->points;
}

void add_stripe_to_preblob(preblob_t *pb, stripe_t *stripe)
{
#ifdef DBG_MSG
  printf("Adding stripe to blob %p\n",pb);
#endif
  pb->sum_x += ((float)stripe->sum * stripe->hstart) + stripe->sum_x;
  pb->sum_y += (float)stripe->sum * stripe->vline;
  pb->sum += stripe->sum;
  pb->points += stripe->points;
}

preblob_t* preblob_from_stripe(stripe_t *stripe)
{
  preblob_t *pb = (preblob_t*)my_malloc(sizeof(preblob_t));
  pb->sum_x = ((float)stripe->sum * stripe->hstart) + stripe->sum_x;
  pb->sum_y = (float)stripe->sum * stripe->vline;
  pb->sum = stripe->sum;
  pb->points = stripe->points;
  pb->added = false;
  pb->matched = true;
#ifdef DBG_MSG
  printf("Creating new blob %p\n",pb);
#endif
  return pb;
}

bool store_preblobs(bool all)
{
  if(preblobs == NULL){
    preblobs = create_list();
  }
  int i;
  for(i = 0; i < current.limit; ++i){
#ifdef DBG_MSG
    printf("Checking current %p m:%c a:%c: ", current.ranges[i].pb,
	   current.ranges[i].pb->matched?'t':'f', current.ranges[i].pb->added?'t':'f');
#endif
    if(!(current.ranges[i].pb->matched)){
      if(!current.ranges[i].pb->added){
#ifdef DBG_MSG
	printf("Added!");
#endif
        current.ranges[i].pb->added = true;
        add_element(preblobs, current.ranges[i].pb);
      }
    }
#ifdef DBG_MSG
    printf("\n");
#endif
  }
  for(i = 0; i < next.limit; ++i){
      next.ranges[i].pb->matched = false;
    if(all){
#ifdef DBG_MSG
      printf("Checking next %p: ", current.ranges[i].pb);
#endif
      if(!next.ranges[i].pb->added){
#ifdef DBG_MSG
	printf("Added!\n");
#endif
        next.ranges[i].pb->added = true;
        add_element(preblobs, next.ranges[i].pb);
      }
    }
#ifdef DBG_MSG
    printf("\n");
#endif
  }
  return true;
}

bool add_stripe(stripe_t *stripe, image *img)
{
  assert(current.ranges != NULL);
  assert(stripe != NULL);
  assert(img != NULL);
  if(img->bitmap != NULL){
    draw_stripe(img, stripe->hstart, stripe->vline, stripe->hstop, 0x80);
  }
#ifdef DBG_MSG
  printf("Adding stripe: y:%d   x:%d - %d (%d   %d)\n", stripe->vline, stripe->hstart, 
         stripe->hstop, stripe->sum, stripe->sum_x);
#endif
  int i;
  //First of all check if we aren't on a different line
  if(current_vline != stripe->vline){
    //Line differs - check if it isn't next line
    if((current_vline + 1) != stripe->vline){
      store_preblobs(true);
      current.limit= 0;
      next.limit = 0;
    }else{
      //I'm on the next line, so put next to current and clean next
/*      printf("Next line!\n");
      for(i = 0; i < next.limit; ++i){
        printf("Next: %d - %d, %p, %s\n", next.ranges[i].x1, next.ranges[i].x2,
               next.ranges[i].pb, next.ranges[i].matched ? "1" : "0");
      }
*/
      store_preblobs(false);
      stripe_array tmp;
      tmp = current;
      current = next;
      next = tmp;
      next.limit = 0;
#ifdef DBG_MSG
      for(i = 0; i < current.limit; ++i){
        printf("Current: %d - %d, %p, %s\n", current.ranges[i].x1, current.ranges[i].x2,
               current.ranges[i].pb, current.ranges[i].pb->matched ? "1" : "0");
      }
#endif
    }
    current_vline = stripe->vline;
  }
  range *rng = NULL;
  for(i = 0; i < current.limit; ++i){
    if(stripe_in_range(stripe, &(current.ranges[i]))){
      current.ranges[i].pb->matched = true;
      if(rng == NULL){
        rng = &(current.ranges[i]);
      }else{
        if(rng->pb != current.ranges[i].pb){
          merge_preblobs(rng->pb, current.ranges[i].pb);
          preblob_t *merged = current.ranges[i].pb;
	  int j;
	  for(j = i; j < current.limit; ++j){
	    if(current.ranges[j].pb == merged){
	      current.ranges[j].pb = rng->pb;
	    }
	  }
	  for(j = 0; j < next.limit; ++j){
	    if(next.ranges[j].pb == merged){
	      next.ranges[j].pb = rng->pb;
	    }
	  }
	  free(merged);
        }
      }
    }
  }
  preblob_t *pb = NULL;
  if(rng == NULL){
    pb = preblob_from_stripe(stripe);
  }else{
    pb = rng->pb;
    add_stripe_to_preblob(pb, stripe);
  }
  range *new_rng = &(next.ranges[next.limit++]);
  new_rng->x1 = stripe->hstart;
  new_rng->x2 = stripe->hstop;
  new_rng->pb = pb;
  return true;
}

void prepare_for_processing(int w, int h)
{
  if(current.ranges == NULL){
    current.ranges = (range*)my_malloc(sizeof(range) * ((w / 2) + 1));
    next.ranges = (range*)my_malloc(sizeof(range) * ((w / 2) + 1));
  }
}

void to_stripes(image *img)
{
  assert(img != NULL);
  int x, y;
  unsigned char *ptr;
  bool in_stripe = false;
  stripe_t stripe;
  
#ifdef DBG_MSG
  printf(">\n");
#endif
  
  for(y = 0; y < img->h; ++y){
    ptr = img->bitmap + (y * img->w);
    for(x = 0; x < img->w; ++x){
      if(*ptr != 0){
        if(in_stripe){
          ++stripe.points;
          stripe.hstop = x;
          stripe.sum += *ptr;
          stripe.sum_x += ((*ptr) * stripe.points);
        }else{
          stripe.points = 0;
          stripe.vline = y;
          stripe.hstart = x;
          stripe.hstop = x;
          stripe.sum_x = 0;
          stripe.sum = *ptr;
          in_stripe = true;
        }
      }else{
        if(in_stripe){
          ++stripe.points;
          in_stripe = false;
          //printf("Stripe: y: %1d, from %1d to %1d, %1d points;\n", 
          //       stripe.vline, stripe.hstart, stripe.hstop, stripe.points);
          //printf("sum: %6d, sum_x: %6d\n", stripe.sum, stripe.sum_x);
          add_stripe(&stripe, img);
        }
      }
      ptr++;
    }
    if(in_stripe){
      ++stripe.points;
      in_stripe = false;
      //printf("Stripe: y: %1d, from %1d to %1d, %1d points;\n", 
      //       stripe.vline, stripe.hstart, stripe.hstop, stripe.points);
      //printf("sum: %6d, sum_x: %6d\n", stripe.sum, stripe.sum_x);
      add_stripe(&stripe, img);
      in_stripe = false;
    }
  }
  //printf("\n");
}

int stripes_to_blobs(int num_blobs, struct bloblist_type *blt, 
		     int min_pts, int max_pts, image *img)
{
  store_preblobs(true);
  current.limit= 0;
  next.limit = 0;
  current_vline = -2;
  if(preblobs == NULL){
    return -1;
  }
  int counter = 0;
  int valid =0;
  iterator i;
  struct blob_type *cal_b;
  preblob_t *pb;
  init_iterator(preblobs, &i);
  while((pb = (preblob_t*)get_next(&i)) != NULL){
    if((pb->points < min_pts) || (pb->points > max_pts)){
      continue;
    }
    ++valid;
    if(counter < num_blobs){
      float x = pb->sum_x / pb->sum;
      float y = pb->sum_y / pb->sum;
      //printf("%f\t\t%f\t\t%d\n", x, y, pb->points);
      cal_b = &(blt->blobs[counter]);
      cal_b->x = ((img->w - 1) / 2.0) - x;
      cal_b->y = ((img->h - 1) / 2.0) - (y * img->ratio);
      if(img->bitmap != NULL){
	draw_cross(img, x, y, (int) img->w/100.0);
      }
      cal_b->score = pb->points;
    }
    ++counter;
  }
  free_list(preblobs, true);
  preblobs = NULL;
  blt->num_blobs = (valid > num_blobs) ? num_blobs : valid;
  return 0;
}




