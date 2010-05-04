#include "list.h"
#include <stdio.h>
#include "usb_ifc.h"
#include "tir_hw.h"
#include "tir_img.h"
#include "utils.h"
#include "image_process.h"
#include <assert.h>

static int pkt_no = 0;
static image *p_img = NULL;

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
    assert(stripe.hstart >= 81);
    assert(stripe.hstop >= 81);
    assert(stripe.vline >= 12);
    stripe.hstart -= 81;
    stripe.vline -= 12;
    stripe.hstop -= 81;
    stripe.sum = stripe.hstop - stripe.hstart + 1;
    stripe.sum_x = (unsigned int)(stripe.sum * (stripe.sum - 1) / 2.0);
    stripe.points = stripe.sum;
    if(!add_stripe(&stripe, p_img)){
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
    if(!add_stripe(&stripe, p_img)){
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


bool process_packet_tir5(unsigned char data[], size_t *ptr, unsigned int pktsize, int limit)
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


bool process_packet_tir4(unsigned char data[], size_t *ptr, int pktsize, unsigned int limit)
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
  static unsigned int limit = -1;
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




int read_blobs_tir(struct bloblist_type *blt, int min, int max, image *img)
{
  assert(blt != NULL);
  assert(img != NULL);
  p_img = img;
  static size_t size = 0;
  static size_t ptr = 0;
  bool have_frame = false;
  while(1){
    if(ptr >= size){
      ptr = 0;
      if(!receive_data(packet, sizeof(packet), &size, 1000)){
	log_message("Problem reading data from USB!\n");
        return -1;
      }
    }
    if((have_frame = process_packet(packet, &ptr, size)) == true){
      break;
    }
  }
  
  if(have_frame){
    int res = stripes_to_blobs(3, blt, min, max, img);
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

