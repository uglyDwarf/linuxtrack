#include "list.h"
#include <stdio.h>
#include "usb_ifc.h"
#include "tir_hw.h"
#include "utils.h"

typedef struct {
  unsigned int vline;
  unsigned int hstart;
  unsigned int hstop;
  unsigned int sum_x;
  unsigned int sum;
} stripe_t;

typedef struct {
  stripe_t last_stripe;
  stripe_t current_stripe;
  int current_stripes;
  float sum_x, sum_y;
  int count;
} preblob_t;

static plist preblobs = NULL;
static plist finished_blobs = NULL;

static void print_pblob(preblob_t *b)
{
  log_message("last:{v:%d %d - %d} current:{v:%d %d - %d} cur:%d\n", b->last_stripe.vline, b->last_stripe.hstart,
    b->last_stripe.hstop, b->current_stripe.vline, b->current_stripe.hstart,
    b->current_stripe.hstop, b->current_stripes);
}

static int stripes_to_blobs_tir()
{
  if(preblobs == NULL){
    preblobs = create_list();
  }
  if(finished_blobs == NULL){
    finished_blobs = create_list();
  }
  int cntr = 0;
  iterator i;
  preblob_t *pb;
  init_iterator(finished_blobs, &i);
  while((pb = (preblob_t*)get_next(&i)) != NULL){
    float x = pb->sum_x / pb->count;
    float y = pb->sum_y / pb->count;
    log_message("Blob: %g   %g   %d points\n", x, y, pb->count);
    cntr++;
  }
  init_iterator(preblobs, &i);
  while((pb = (preblob_t*)get_next(&i)) != NULL){
    float x = pb->sum_x / pb->count;
    float y = pb->sum_y / pb->count;
    
    log_message("Blob: %g   %g   %d points\n", x, y, pb->count);
    cntr++;
  }
  free_list(preblobs, true);
  preblobs = NULL;
  free_list(finished_blobs, true);
  finished_blobs = NULL;
  
  if(cntr == 3){
//    turn_led_on_tir(TIR_LED_GREEN);
  }else{
//    turn_led_off_tir(TIR_LED_GREEN);
  }
  log_message("%d\n", cntr);
  return 0;
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
}

static void add_stripe_to_preblob(preblob_t *pb, stripe_t *stripe)
{
  float num = stripe->hstop - stripe->hstart + 1;
  pb->sum_x += ((stripe->hstart + stripe->hstop) * (num / 2.0));
  pb->sum_y += (stripe->vline * num);
  pb->count += num;
  pb->current_stripes++;
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
//  log_message("Stripe: %d   %d - %d\n", stripe->vline, stripe->hstart, stripe->hstop);
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




static int process_stripes_tir4(int size, unsigned char payload[])
{
  int i;
  stripe_t stripe;
  unsigned char rest;
  bool null_packet = false;
//  log_message("%d\n", size);
  for(i = 0; i < size; i += 4){
    if(*(unsigned int*)(&(payload[i])) == 0){
      null_packet = true;
      break;
    }
    stripe.vline = payload[i];
    stripe.hstart = payload[i + 1];
    stripe.hstop = payload[i + 2];
    rest = payload[i + 3];
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
    stripe.sum = stripe.hstop - stripe.hstart + 1;
    stripe.sum_x = (unsigned int)(stripe.sum * (stripe.sum - 1) / 2.0);
    
    if(add_stripe(&stripe)){
      log_message("Couldn't add stripe!\n");
    }
  }
  if(!null_packet){
    stripes_to_blobs_tir();
//    log_message("End of frame!\n");
  }
  return 0;
}

static int process_stripes_tir5(int size, unsigned char payload[])
{
  int i;
  stripe_t stripe;
  bool null_packet = false;
  for(i = 0; i < size; i += 8){
    if(*(unsigned int*)(&(payload[i])) == 0){
      null_packet = true;
      break;
    }
    stripe.hstart = (((unsigned int)payload[i]) << 2) | 
                     (((unsigned int)payload[i+1]) >> 6);
    stripe.vline = ((((unsigned int)payload[i+1]) & 0x3F) << 3) | 
                    ((((unsigned int)payload[i+2]) & 0xE0) >> 5);
    stripe.hstop = (((((unsigned int)payload[i+2]) & 0x3F) << 3) | 
                    (((unsigned int)payload[i+3]) >> 3)) + stripe.hstart;
    stripe.sum_x = (((unsigned int)payload[i+3]) & 7) << 17 |
                    (((unsigned int)payload[i+4]) << 9) | 
		    ((unsigned int)payload[i+5]) << 1 |
		    ((unsigned int)payload[i+6]) >>7;
    stripe.sum = (((unsigned int)payload[i+6]) & 7) << 8 |
                   ((unsigned int)payload[i+7]);
    
    if(add_stripe(&stripe)){
      log_message("Couldn't add stripe!\n");
    }
  }
  if(!null_packet){
    stripes_to_blobs_tir();
    log_message("End of frame!\n");
  }
  return 0;
}

bool process_packet_tir5(unsigned char data[], size_t size)
{
  unsigned char packet_num = data[0];
  if(data[1] != 0x10){
    log_message("Unrecognized packet number 0x%02X!\n", packet_num);
    return false;
  }
  if((data[0] ^ data[1] ^ data[2] ^ data[3]) != 0xAA){
    log_message("Bad packet header!\n");
    return false;
  }
  unsigned int ts = (((((data[size-4] << 8) + data[size-3]) << 8) + data[size-2])
    << 8) + data[size-1];
  if(ts != (size-8)){
    log_message("Bad packet length! (received %d, indicated %d)\n", size - 8, ts);
  }
  switch(data[2]){
    case 0:
      process_stripes_tir4(size - 8, &(data[4]));
      break;
    case 5:
      process_stripes_tir5(size - 8, &(data[4]));
      break;
  }
  return true;
}

int read_packet_tir()
{
  size_t res, transferred;
  if(!receive_data(packet, sizeof(packet), &transferred)){
    log_message("Problem reading data from TIR! %d - %d transferred\n", res, transferred);
    return 1;
  }
  if(packet[1] == 0x1C){
    int limit = (transferred > packet[0]) ? packet[0] : transferred;
    process_stripes_tir4(limit-2, &(packet[2]));
  }else if(packet[1] == 0x7e){
    log_message("Nop packet!\n");
  }else if(packet[1] == 0x10){
    process_packet_tir5(packet, transferred);
  }else{
    int i;
    for(i = 0; i < transferred; ++i){
      log_message("%02X ", packet[i]);
    }
    log_message("\n");
  }
  return 0;
}

/*
int main(int argc, char *argv[])
{
  unsigned char buf_t5[] = { 
 0x05, 0x10, 0x05, 0xBA, 0x4B, 0xDD, 0xA0, 0x28, 0x00, 0xA9, 0x80, 0x93, 0x48, 0x5D, 0xC0, 0xA0,
 0x1A, 0x93, 0x04, 0xB1, 0x48, 0x1D, 0xE0, 0xB0, 0x53, 0x83, 0x0E, 0x95, 0x47, 0xDE, 0x00, 0xC0,
 0x76, 0xCD, 0x94, 0xB6, 0x47, 0xDE, 0x20, 0xC0, 0x7B, 0x9A, 0x15, 0x9E, 0x47, 0xDE, 0x40, 0xC0,
 0x7C, 0x1C, 0x15, 0xC7, 0x47, 0xDE, 0x60, 0xC0, 0x7B, 0xFA, 0x15, 0xC2, 0x47, 0xDE, 0x80, 0xC0,
 0x7A, 0xDE, 0x95, 0x7A, 0x47, 0xDE, 0xA0, 0xC0, 0x76, 0xE2, 0x94, 0xBE, 0x48, 0x1E, 0xC0, 0xB0,
 0x65, 0x4F, 0x13, 0x33, 0x48, 0x5E, 0xE0, 0xA0, 0x4F, 0xA7, 0x90, 0xB7, 0x48, 0x9F, 0x00, 0x90,
 0x2B, 0x58, 0x0A, 0x79, 0x49, 0x1F, 0x20, 0x70, 0x0A, 0x9F, 0x83, 0x71, 0x53, 0xA9, 0x00, 0x28,
 0x00, 0x6E, 0x80, 0x70, 0x43, 0xA9, 0x20, 0x30, 0x01, 0xD6, 0x01, 0x1D, 0x53, 0x69, 0x20, 0x50,
 0x04, 0x25, 0x82, 0x96, 0x57, 0x69, 0x20, 0x28, 0x00, 0x79, 0x00, 0x6E, 0x42, 0xE9, 0x40, 0x50,
 0x0B, 0xAA, 0x83, 0xF4, 0x53, 0x29, 0x40, 0xB8, 0x29, 0x36, 0x09, 0xDB, 0x40, 0x29, 0x60, 0xB0,
 0x40, 0xC0, 0x88, 0xEE, 0x52, 0xE9, 0x60, 0xC8, 0x77, 0x01, 0x93, 0xCA, 0x3F, 0xE9, 0x80, 0xC0,
 0x67, 0x24, 0x90, 0xA4, 0x52, 0xE9, 0x80, 0xD0, 0x8C, 0x25, 0x96, 0x8B, 0x3F, 0xA9, 0xA0, 0xC8,
 0x84, 0x5A, 0x16, 0x95, 0x52, 0xE9, 0xA0, 0xD0, 0x91, 0x23, 0x97, 0x02, 0x3F, 0x69, 0xC0, 0xD0,
 0x91, 0x33, 0x17, 0x60, 0x52, 0xE9, 0xC0, 0xD0, 0x92, 0x6F, 0x97, 0x26, 0x3F, 0x69, 0xE0, 0xD0,
 0x91, 0x0E, 0x97, 0x8D, 0x52, 0xE9, 0xE0, 0xD0, 0x92, 0xC9, 0x97, 0x3B, 0x3F, 0x6A, 0x00, 0xD0,
 0x8F, 0x9A, 0x97, 0x74, 0x52, 0xEA, 0x00, 0xD0, 0x92, 0x96, 0x97, 0x35, 0x3F, 0x6A, 0x20, 0xD0,
 0x8B, 0x3A, 0x96, 0xFE, 0x52, 0xEA, 0x20, 0xD0, 0x90, 0xC1, 0x16, 0xD0, 0x3F, 0x6A, 0x40, 0xC8,
 0x85, 0x94, 0x96, 0x31, 0x53, 0x2A, 0x40, 0xC8, 0x81, 0xB1, 0x15, 0xF7, 0x3F, 0xAA, 0x60, 0xC0,
 0x71, 0xE5, 0x94, 0xC6, 0x53, 0x2A, 0x60, 0xC0, 0x7B, 0x94, 0x94, 0xB0, 0x3F, 0xAA, 0x80, 0xB8,
 0x63, 0x2A, 0x92, 0xCC, 0x53, 0x6A, 0x80, 0xB8, 0x66, 0x33, 0x11, 0xEF, 0x3F, 0xEA, 0xA0, 0xA8,
 0x45, 0x42, 0x0F, 0x39, 0x53, 0xAA, 0xA0, 0xA8, 0x3A, 0x02, 0x0A, 0xC7, 0x40, 0x2A, 0xC0, 0x90,
 0x1F, 0xE1, 0x08, 0x4C, 0x54, 0x6A, 0xC0, 0x80, 0x0D, 0x23, 0x03, 0x64, 0x40, 0xAA, 0xE0, 0x60,
 0x06, 0xA7, 0x02, 0x78, 0x00, 0x00, 0x01, 0x60
  };
  unsigned char buf_t4[] = {
 0x3e, 0x1c, 0xb2, 0x31, 0x35, 0x18, 0xb3, 0x2f,
 0x40, 0x18, 0xb4, 0x2f, 0x42, 0x18, 0xb5, 0x2f, 
 0x42, 0x18, 0xb6, 0x31, 0x41, 0x18, 0xb7, 0x35, 
 0x3e, 0x18, 0xd7, 0x07, 0x17, 0x18, 0xd8, 0x06, 
 0x1a, 0x18, 0xd9, 0x06, 0x1b, 0x18, 0xd9, 0x4f, 
 0x53, 0x18, 0xda, 0x07, 0x1b, 0x18, 0xda, 0x4d, 
 0x5f, 0x18, 0xdb, 0x08, 0x1a, 0x18, 0xdb, 0x4c, 
 0x61, 0x18, 0xdc, 0x0c, 0x18, 0x18, 0x7f, 0x2e
  };
  process_packet_tir5(buf_t5, sizeof(buf_t5));
  process_packet_tir4(&(buf_t4[2]), sizeof(buf_t4-4));
  return 0;
}
*/
