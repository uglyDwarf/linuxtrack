#include "list.h"
#include <stdio.h>
#include "usb_ifc.h"
#include "tir_hw.h"
#include "tir_img.h"
#include "utils.h"
#include <assert.h>

typedef struct {
  unsigned int vline;
  unsigned int hstart;
  unsigned int hstop;
  unsigned int sum_x;
  unsigned int sum;
  unsigned int points;
} stripe_t;

typedef struct {
  stripe_t last_stripe;
  stripe_t current_stripe;
  int current_stripes;
  float sum_x, sum_y;
  int count;
  int points;
} preblob_t;

static plist preblobs = NULL;
static plist finished_blobs = NULL;
static unsigned char *picture = NULL;
static unsigned int pic_x, pic_y;
static float pic_yf;
static int pkt_no = 0;
/*
static void print_pblob(preblob_t *b)
{
  log_message("last:{v:%d %d - %d} current:{v:%d %d - %d} cur:%d\n", b->last_stripe.vline, b->last_stripe.hstart,
    b->last_stripe.hstop, b->current_stripe.vline, b->current_stripe.hstart,
    b->current_stripe.hstop, b->current_stripes);
}
*/


blob* new_blob(float x, float y, float score)
{
  blob *tmp = (blob*)my_malloc(sizeof(blob));
  tmp->x = x;
  tmp->y = y;
  tmp->score = score;
  return tmp;
}

static void clip_coord(int *coord, int min,
                       int max)
{
  int tmp = *coord;
  tmp = (tmp < min) ? min : tmp;
  tmp = (tmp > max) ? max : tmp;
  *coord = tmp;
}

static void draw_stripe(int x, int y, int x_end, unsigned char color)
{
  if(picture == NULL)
    return;
  clip_coord(&x, 0, pic_x);
  clip_coord(&y, 0, pic_y);
  clip_coord(&x_end, 0, pic_x);
  unsigned char *ptr = picture + y * pic_x + x;
  x_end -= x;
  while(x_end > 0){
    *(++ptr) = color;
    --x_end;
  }
}

void draw_cross(int x, int y)
{
  int x1 = (int)x - 5;
  int x2 = (int)x + 5;
  int y1 = y - 5;
  int y2 = y + 5;
  
  if(picture == NULL)
    return;
  clip_coord(&x1, 0, pic_x);
  clip_coord(&y1, 0, pic_y);
  clip_coord(&x2, 0, pic_x);
  clip_coord(&y2, 0, pic_y);
  
  draw_stripe(x1, y, x2, 0xFF);
  
  unsigned char *ptr = picture + y1 * pic_x + x;
  int count = y2 - y1 + 1;
  while(count > 0){
    *ptr = 0xf0;
    ptr += pic_x;
    --count;
  }
}

static int stripes_to_blobs_tir(plist *blob_list)
{
  if(preblobs == NULL){
    preblobs = create_list();
  }
  if(finished_blobs == NULL){
    finished_blobs = create_list();
  }
  plist blobs = create_list();
  int cntr = 0;
  iterator i;
  preblob_t *pb;
  init_iterator(finished_blobs, &i);
  while((pb = (preblob_t*)get_next(&i)) != NULL){
    float x = pb->sum_x / pb->count;
    float y = (pb->sum_y / pb->count) * pic_yf;
    draw_cross(x, y);
    add_element(blobs, new_blob(x, y, pb->points));
    //printf("Blob: %g   %g   %d points\n", x, y, pb->count);
    cntr++;
  }
  init_iterator(preblobs, &i);
  while((pb = (preblob_t*)get_next(&i)) != NULL){
    float x = pb->sum_x / pb->count;
    float y = (pb->sum_y / pb->count) * pic_yf;
    draw_cross(x, y);
    add_element(blobs, new_blob(x, y, pb->points));
    //printf("Blob: %g   %g   %d points\n", x, y, pb->count);
    cntr++;
  }
  //log_message("End of blobs %d!\n", cntr);
  free_list(preblobs, true);
  preblobs = NULL;
  free_list(finished_blobs, true);
  finished_blobs = NULL;
  *blob_list = blobs;
  return cntr;
}

static bool stripes_coincide(stripe_t *stripe1, stripe_t *stripe2)
{
  if(stripe2->vline - stripe1->vline == 1){
    if((stripe1->hstart < stripe2->hstop) && 
      (stripe1->hstop > stripe2->hstart)){
      return true;
    }
  }
  return false;
}

static void merge_blobs(preblob_t *b1, preblob_t *b2)
{
  b1->last_stripe.hstop = b2->last_stripe.hstop;
  b1->sum_x += b2->sum_x;
  b1->sum_y += b2->sum_y;
  b1->count += b2->count;
  b1->points += b2->points;
}

static void add_stripe_to_preblob(preblob_t *pb, stripe_t *stripe)
{
  pb->sum_x += ((float)stripe->sum * stripe->hstart) + stripe->sum_x;
  pb->sum_y += (float)stripe->sum * stripe->vline;
  pb->count += stripe->sum;
  pb->current_stripes++;
  pb->points += stripe->points;
  if(pb->current_stripes == 1){
    pb->current_stripe = *stripe;
  }else{
    pb->current_stripe.hstop = stripe->hstop;
  }
}

static preblob_t* new_preblob()
{
  preblob_t* pb = (preblob_t*) my_malloc(sizeof(preblob_t));
  pb->sum_x = pb->sum_y = 0.0f;
  pb->count = 0;
  pb->points = 0;
  pb->current_stripes = 0;
  pb->last_stripe.vline = -1;
  return pb;
}

static void add_to_list(plist *l, void* elem)
{
  if(*l == NULL){
    *l = create_list();
  }
  add_element(*l, elem);
}


static int add_stripe(stripe_t *stripe)
{
  //log_message("Stripe: %d   %d - %d (%d   %d)\n", stripe->vline, stripe->hstart, 
  //       stripe->hstop, stripe->sum, stripe->sum_x);
  draw_stripe(stripe->hstart, stripe->vline * pic_yf, stripe->hstop, 0x80);
  if(preblobs == NULL){
    preblobs = create_list();
  }
  iterator i;
  preblob_t *pb;
  preblob_t *first_match = NULL;
  static int last_vline = -1;
  //moving to the new line
  if(last_vline != stripe->vline){
    last_vline = stripe->vline;
    init_iterator(preblobs, &i);
    while((pb = (preblob_t*)get_next(&i)) != NULL){
      pb->last_stripe = pb->current_stripe;
      pb->current_stripes = 0;
      //move finished blobs
      if(stripe->vline - pb->last_stripe.vline > 1){
	add_to_list(&finished_blobs, (void*)pb);
	delete_current(preblobs, &i);
	continue;
      }
    }
  }
  
  init_iterator(preblobs, &i);
  while((pb = (preblob_t*)get_next(&i)) != NULL){
    if(stripes_coincide(&(pb->last_stripe), stripe)){
      if(first_match == NULL){
        first_match = pb;
        add_stripe_to_preblob(pb, stripe);
      }else{
        merge_blobs(first_match, pb);
	free(delete_current(preblobs, &i));
      }
    }
  }
  if(first_match == NULL){
    pb = new_preblob();
    add_element(preblobs, pb);
    add_stripe_to_preblob(pb, stripe);
  }
  return 0;
}

static bool process_stripe_tir(unsigned char p_stripe[])
{
  stripe_t stripe;
  unsigned char rest;
  
    stripe.vline = p_stripe[0];
    stripe.hstart = p_stripe[1];
    stripe.hstop = p_stripe[2];
    rest = p_stripe[3];
    if(rest & 0x20)
      stripe.vline |= 0x100;
    if(rest & 0x80)
      stripe.hstart |= 0x100;
    if(rest & 0x40)
      stripe.hstop |= 0x100;
    if(rest & 0x10)
      stripe.hstart |= 0x200;
    if(rest & 0x08)
      stripe.hstop |= 0x200;
    stripe.hstart -= 82;
    stripe.vline -= 12;
    stripe.hstop -= 82;
    stripe.sum = stripe.hstop - stripe.hstart + 1;
    stripe.sum_x = (unsigned int)(stripe.sum * (stripe.sum - 1) / 2.0);
    stripe.points = stripe.sum;
    if(add_stripe(&stripe)){
      log_message("Couldn't add stripe!\n");
    }
  return true;
}

static bool process_stripe_tir5(unsigned char payload[])
{
  stripe_t stripe;
    stripe.hstart = (((unsigned int)payload[0]) << 2) | 
                     (((unsigned int)payload[1]) >> 6);
    stripe.vline = ((((unsigned int)payload[1]) & 0x3F) << 3) | 
                    ((((unsigned int)payload[2]) & 0xE0) >> 5);
    stripe.points = (((((unsigned int)payload[2]) & 0x1F) << 5) | 
                    (((unsigned int)payload[3]) >> 3));
    stripe.hstop =  stripe.points + stripe.hstart - 1;
    stripe.sum_x = (((unsigned int)payload[3]) & 7) << 17 |
                    (((unsigned int)payload[4]) << 9) | 
		    ((unsigned int)payload[5]) << 1 |
		    ((unsigned int)payload[6]) >>7;
    stripe.sum = (((unsigned int)payload[6]) & 0x7F) << 8 |
                   ((unsigned int)payload[7]);
    if(add_stripe(&stripe)){
      log_message("Couldn't add stripe!\n");
    }
  return true;
}

bool check_paket_header_tir5(unsigned char data[])
{
  if((data[0] ^ data[1] ^ data[2] ^ data[3]) != 0xAA){
    log_message("Bad packet header!\n");
    return false;
  }else{
    return true;
  }
}


bool process_packet_tir5(unsigned char data[], size_t *ptr, int pktsize, int limit)
{
  bool have_frame = false;
  unsigned int ps = 0;
  unsigned char type = data[*ptr + 2];

  if((type == 0) || (type == 5)){
    if(!check_paket_header_tir5(&(data[*ptr]))){
      log_message("Bad packet header!\n");
      assert(0);
      return false;
    }
    ps = data[limit - 4];
    ps = (ps << 8) + data[limit - 3];
    ps = (ps << 8) + data[limit - 2];
    ps = (ps << 8) + data[limit - 1];
    if(ps != (pktsize - 8)){
      log_message("Bad packet size! %d x %d\n", ps, pktsize - 8);
//      assert(0);
      return false;
    }
  }
  
  switch(type){
    case 0:
    case 5:
      pkt_no = data[*ptr];
      *ptr += 4;
      while(ps > 0){
	if(type == 0){
	  process_stripe_tir((unsigned char *)&(data[*ptr]));
	  *ptr += 4;
          ps -= 4;
	}else{
	  process_stripe_tir5((unsigned char *)&(data[*ptr]));
	  *ptr += 8;
          ps -= 8;
	}
      }
      have_frame = true;
      break;
    case 3:
      //These are most probably B/W data from camera without any preprocessing..
      //We ignore them.
      break;
  }
  return have_frame;
}


bool process_packet_tir4(unsigned char data[], size_t *ptr, int pktsize, int limit)
{
  unsigned int *ui;
  bool have_frame = false;

  ui = (unsigned int *)&(data[*ptr]);
  bool go_on = true;
  do{
    if(*ui == 0){
      have_frame = true;
      go_on = false;
    }else{
//      log_message("\t%02X%02X%02X%02X\n", data[*ptr], data[*ptr + 1],
//             data[*ptr + 2], data[*ptr + 3]);
      assert((data[*ptr + 3] & 7) == 0);
      process_stripe_tir((unsigned char *)ui);
    }
    ++ui;
    (*ptr) += 4;
    if(*ptr >= limit){
//      log_message(">>>  size %d, limit %d, ptr %d\n", pktsize, limit, *ptr);
      go_on = false;
      if(pktsize == 0x3E){
	*ptr += 2;
      }
    }
  }while(go_on);
  return have_frame;
}


bool process_packet(unsigned char data[], size_t *ptr, size_t size)
{
  static int type = -1;
  static int limit = -1;
  static int pktsize = 0;
  bool have_frame;
  
//  log_message("Reading packet!\n");
  while(1){
    if(*ptr >= size){
      return false;
    }
    have_frame = false;
    if(type == -1){
      type = data[(*ptr) + 1];
//      log_message("Packet type %02X\n", type);
      switch(type){
        case 0x20:
        case 0x40:
        case 0x1C:
          pktsize = data[*ptr];
          limit= (*ptr) + pktsize;
          *ptr += 2;
          break;
        case 0x10:
          if((data[(*ptr) + 2] == 0) || (data[(*ptr) + 2] == 5)){
            pktsize = size;
            limit= (*ptr) + pktsize;
          }else{
            pktsize = data[*ptr];
            limit= (*ptr) + pktsize;
          }
          break;
        default:
          log_message("ERROR!!! ('%02X %02X')\n", data[*ptr], data[*ptr + 1]);
/*	  printf("Error at %d\n", *ptr);
	  int counter;
	  for(counter = 0; counter < size+2; ++counter){
	    if(counter % 16 == 0){
	      printf("\n%6d ", counter);
	    }
	    printf("%02X ", data[counter]);
	  }
	  printf("\n");
*/
	  *ptr = size; //Read new packet...
          return false;
          break;
      }
    }
//    log_message("size %d, limit %d, ptr %d\n", pktsize, limit, *ptr);
    if(limit > size){
      break;
    }
    switch(type){
      case 0x20:
        //Status packet
        *ptr = limit;
        type = -1;
        break;
      case 0x40:
        //DevInfo packet
        *ptr = limit;
        type = -1;
        break;
      case 0x10:
        //TIR5 packet
        have_frame = process_packet_tir5(data, ptr, pktsize, limit);
        *ptr = limit;
        type = -1;
        break;
      case 0x1C:
        //TIR4 packet
        have_frame = process_packet_tir4(data, ptr, pktsize, limit);
	if(*ptr >= limit){
          type = -1;
	}
        //log_message("   size %d, limit %d, ptr %d\n", pktsize, limit, *ptr);
        break;
      default:
        break;
    }

    if(have_frame == true){
      break;
    }
  }
  return have_frame;
}




int read_blobs_tir(plist *blob_list, unsigned char pic[], 
                   unsigned int x, unsigned int y, float yf)
{
  static size_t size = 0;
  static size_t ptr = 0;
  bool have_frame = false;
  picture = pic;
  pic_x = x;
  pic_y = y;
  pic_yf = yf;
  while(1){
    if(ptr >= size){
      ptr = 0;
      if(!receive_data(packet, sizeof(packet), &size, 1000)){
        return -1;
      }
    }
    if((have_frame = process_packet(packet, &ptr, size)) == true){
      break;
    }
  }
  
  if(have_frame){
    int res = stripes_to_blobs_tir(blob_list);
/*    
    if(pic != NULL){
      static int fc = 0;
      char name[] = "fXXXXXXX.raw";
      sprintf(name, "f%02X%04d.raw", pkt_no, fc++);
      printf("%s\n", name);
      FILE *f = fopen(name, "wb");
      if(f != NULL){
	fwrite(pic, 1, x * y, f);
	fclose(f);
      }
    }
*/
    return res;
  }else{
    return 0;
  }
}

