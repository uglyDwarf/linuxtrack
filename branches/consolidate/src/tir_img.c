#include "list.h"
#include <stdio.h>
#include "usb_ifc.h"
#include "tir_hw.h"
#include "tir_img.h"
#include "utils.h"
#include "image_process.h"
#include "sn4_com.h"
#include <assert.h>

static unsigned int pkt_no = 0;
static image *p_img = NULL;
static unsigned int current_line = 0;
static dev_found device = NOT_TIR;

static bool process_stripe_tir2(unsigned char p_stripe[])
{
  stripe_t stripe;
    stripe.vline = p_stripe[0];
    stripe.hstart = p_stripe[1];
    stripe.hstop = p_stripe[2];
    stripe.sum = stripe.hstop - stripe.hstart + 1;
    stripe.sum_x = (unsigned int)(stripe.sum * (stripe.sum - 1) / 2.0);
    stripe.points = stripe.sum;
    if(!ltr_int_add_stripe(&stripe, p_img)){
      ltr_int_log_message("Couldn't add stripe!\n");
    }
  return true;
}
static bool process_stripe_sn4(unsigned char p_stripe[])
{
  stripe_t stripe;
  unsigned char rest;
  
    stripe.vline = p_stripe[0];
    stripe.hstart = p_stripe[1];
    stripe.hstop = p_stripe[2];
    rest = p_stripe[3];
    if(rest & 0x01)
      stripe.hstop |= 0x100;
    if(rest & 0x02)
      stripe.hstart |= 0x100;
    if(rest & 0x04)
      stripe.vline |= 0x100;
    if(rest & 0x08)
      stripe.hstop |= 0x200;
    if(rest & 0x10)
      stripe.hstart |= 0x200;
    if(rest & 0x20)
      stripe.vline |= 0x200;
    if(rest & 0x40)
      stripe.hstop |= 0x400;
    if(rest & 0x80)
      stripe.hstart |= 0x400;
    stripe.sum = stripe.hstop - stripe.hstart + 1;
    stripe.sum_x = (unsigned int)(stripe.sum * (stripe.sum - 1) / 2.0);
    stripe.points = stripe.sum;
    if(!ltr_int_add_stripe(&stripe, p_img)){
      ltr_int_log_message("Couldn't add stripe!\n");
    }
  return true;
}

static bool process_stripe_sn4gr(unsigned char p_stripe[], size_t size)
{
  stripe_t stripe;
  unsigned char rest;
  size_t i, j;
  unsigned int last_vline = 0;
    
  if(size < 4){
    return false;
  }
  i = 0;
  
  while(i < (size - 4)){
    stripe.hstart = p_stripe[i];
    stripe.vline = p_stripe[i + 1];
    rest = p_stripe[i + 2];
    if(rest & 0x01)
      stripe.hstart |= 0x100;
    if(rest & 0x02)
      stripe.hstart |= 0x200;
    if(rest & 0x04)
      stripe.vline |= 0x100;
    stripe.sum = stripe.sum_x = 0;
    if(stripe.vline < last_vline){
//      printf("%d x %d!!!!\n", stripe.vline, last_vline);
//      for(i = 0; i < size; ++i){
//        printf("%02X ", p_stripe[i]);
//      }
      break;
    }
    last_vline = stripe.vline;
    i += 3;
    j = 1;
    while((p_stripe[i] != 0)&&(i < size)){
      stripe.sum += p_stripe[i];
      stripe.sum_x += (j * p_stripe[i]);
      ++i;
      ++j;
    }
    stripe.points = j - 1;
    stripe.hstop = stripe.hstart + stripe.points - 1;
    if(p_stripe[i] == 0){
      ++i;
      //printf("%ld x (%d %d) y %d (%d points)\n", i, stripe.hstart, 
      //  stripe.hstop, stripe.vline, stripe.points);
      if(!ltr_int_add_stripe(&stripe, p_img)){
        ltr_int_log_message("Couldn't add stripe!\n");
      }
    }else{
      //reached the end of packet...
      //printf(">> %02X <<(%ld %ld)\n", p_stripe[i], i, size);
      break;
    }
    
  }  
  return true;
}



static bool process_stripe_tir4(unsigned char p_stripe[])
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
//    assert(stripe.hstart >= 81);
//    assert(stripe.hstop >= 81);
//    assert(stripe.vline >= 12);
//    stripe.hstart -= 81;
//    stripe.vline -= 12;
//    stripe.hstop -= 81;
    stripe.sum = stripe.hstop - stripe.hstart + 1;
    stripe.sum_x = (unsigned int)(stripe.sum * (stripe.sum - 1) / 2.0);
    stripe.points = stripe.sum;
    if(!ltr_int_add_stripe(&stripe, p_img)){
      ltr_int_log_message("Couldn't add stripe!\n");
    }
//    ltr_int_log_message("Stripe: %d\t\t%d\t\t%d %d %d %d\n", stripe.hstart, stripe.hstop, stripe.vline, stripe.sum,
//           stripe.sum_x, stripe.points);
  return true;
}

static bool is_next_frame_tir(unsigned char p_stripe[])
{
  unsigned int vline = p_stripe[0];
  bool res;
  if(p_stripe[3] & 0x20)
    vline |= 0x100;
  if(vline < current_line){
    res = true;
  }else{
    res = false;
  }
  current_line = vline;
  return res;
}

static bool is_next_frame_tir2(unsigned char p_stripe[])
{
  unsigned int vline = p_stripe[0];
  bool res;
  if(vline < current_line){
    res = true;
  }else{
    res = false;
  }
  current_line = vline;
  return res;
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
    if(!ltr_int_add_stripe(&stripe, p_img)){
      ltr_int_log_message("Couldn't add stripe!\n");
    }
  return true;
}

static bool check_paket_header_tir5(unsigned char data[])
{
  if((data[0] ^ data[1] ^ data[2] ^ data[3]) != 0xAA){
    ltr_int_log_message("Bad packet header!\n");
    return false;
  }else{
    return true;
  }
}

static bool check_paket_header_sn4(unsigned char data[])
{
  unsigned char csum = 0;
  int i;
  for(i = 0; i < 8; ++i){
    csum ^= data[i];
  }
  if(csum != 0xAA){
    ltr_int_log_message("Bad packet header!\n");
    return false;
  }else{
    return true;
  }
}


static bool process_packet_tir5(unsigned char data[], size_t *ptr, unsigned int pktsize, int limit)
{
  bool have_frame = false;
  unsigned int ps = 0;
  unsigned char type = data[*ptr + 2];

  if((type == 0) || (type == 5)){
    if((limit < 4) || !check_paket_header_tir5(&(data[*ptr]))){
      ltr_int_log_message("Bad packet header!\n");
      return false;
    }
    ps = data[limit - 4];
    ps = (ps << 8) + data[limit - 3];
    ps = (ps << 8) + data[limit - 2];
    ps = (ps << 8) + data[limit - 1];
    if(ps != (pktsize - 8)){
      ltr_int_log_message("Bad packet size! %d x %d\n", ps, pktsize - 8);
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
	  process_stripe_tir4((unsigned char *)&(data[*ptr]));
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

static bool process_packet_sn4(unsigned char data[], size_t *ptr, unsigned int pktsize, int limit)
{
  bool have_frame = false;
  unsigned int ps = 0;
  static unsigned char prev_btns = 3; 
  unsigned char btns = data[*ptr];
  unsigned char type = data[*ptr + 1];

  if((limit < 12) || !check_paket_header_sn4(&(data[*ptr]))){
    *ptr += limit;
    return false;
  }
  
  ps = data[limit - 4];
  ps = (ps << 8) + data[limit - 3];
  ps = (ps << 8) + data[limit - 2];
  ps = (ps << 8) + data[limit - 1];
  if(ps != (pktsize - 4)){
    *ptr += limit;
    ltr_int_log_message("Bad packet size! %d x %d\n", ps, pktsize - 8);
    return false;
  }
  
  if(btns != prev_btns){
    sn4_btn_event_t ev;
    ev.btns = btns;
    prev_btns = btns;
    gettimeofday(&(ev.timestamp), NULL);
    ltr_int_send_sn4_data((void*)&ev, sizeof(ev));
//    printf("Sending %d!\n", btns);
  }
  
  ps -= 8; // header
  switch(type){
    case 4:
      if(ps > 8){
        ps -= 4; //skip threshold
        *ptr += (4 + 8); //header + threshold
        have_frame = process_stripe_sn4gr((unsigned char *)&(data[*ptr]), ps);
      }else{
        have_frame = false;
      }
      *ptr += limit;
      break;
    case 0:
      pkt_no = data[*ptr+4];
      pkt_no = (pkt_no << 8) + data[*ptr+5];
      pkt_no = (pkt_no << 8) + data[*ptr+6];
      pkt_no = (pkt_no << 8) + data[*ptr+7];
      //printf("%d\n", pkt_no);
      *ptr += 8;
      while(ps > 0){
        process_stripe_sn4((unsigned char *)&(data[*ptr]));
	*ptr += 4;
        ps -= 4;
      }
      *ptr += 4;
      have_frame = true;
      break;
    default:
      *ptr += limit;
      break;
  }
  return have_frame;
}

static bool process_packet_tir4(unsigned char data[], size_t *ptr, int pktsize, unsigned int limit)
{
  (void) pktsize;
  unsigned int *ui;
  bool have_frame = false;

  ui = (unsigned int *)&(data[*ptr]);
  bool go_on = true;
  do{
    if(*ui == 0){
      current_line = 0;
      have_frame = true;
      go_on = false;
      ++ui;
      (*ptr) += 4;
    }else if(is_next_frame_tir((unsigned char *)ui)){
      have_frame = true;
      go_on = false;
    }else{
//      log_message("\t%02X%02X%02X%02X\n", data[*ptr], data[*ptr + 1],
//             data[*ptr + 2], data[*ptr + 3]);
//!!!!      assert((data[*ptr + 3] & 7) == 0);
      process_stripe_tir4((unsigned char *)ui);
      ++ui;
      (*ptr) += 4;
    }
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


static bool process_packet_tir2(unsigned char data[], size_t *ptr, int pktsize, unsigned int limit)
{
  (void) pktsize;
  uint8_t *ui;
  bool have_frame = false;

  ui = (uint8_t *)&(data[*ptr]);
  bool go_on = true;
  do{
    if((ui[0] == 0) && (ui[1] == 0) && (ui[2] == 0)){
      current_line = 0;
      have_frame = true;
      go_on = false;
      ui += 3;
      (*ptr) += 3;
    }else if(is_next_frame_tir2(ui)){
      ltr_int_log_message("Have frame!!!!!!\n");
      have_frame = true;
      go_on = false;
    }else{
//      log_message("\t%02X%02X%02X%02X\n", data[*ptr], data[*ptr + 1],
//             data[*ptr + 2], data[*ptr + 3]);
//!!!!      assert((data[*ptr + 3] & 7) == 0);
      process_stripe_tir2((unsigned char *)ui);
      ui += 3;
      (*ptr) += 3;
    }
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
        case 0x00:
        case 0x04:
          pktsize = size;
          limit= (*ptr) + pktsize;
          break;
          
        default:
          ltr_int_log_message("ERROR!!! ('%02X %02X')\n", data[*ptr], data[*ptr + 1]);
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
          type = -1;
	  *ptr = size; //Read new packet...
          return false;
          break;
      }
    }
    if(limit > size){
      break; //we're supposed to read beyond read-in data...
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
        if(device == TIR2){
          have_frame = process_packet_tir2(data, ptr, pktsize, limit);
        }else{
          have_frame = process_packet_tir4(data, ptr, pktsize, limit);
        }
	if(*ptr >= limit){
          type = -1;
	}
        //log_message("   size %d, limit %d, ptr %d\n", pktsize, limit, *ptr);
        break;
      case 0x04: //SmartNav4 greyscale data
      case 0x00: //SmartNav4 normal data
        have_frame = process_packet_sn4(data, ptr, pktsize, limit);
        *ptr = limit;
        type = -1;
        break;
      default:
        type = -1;
        break;
    }

    if(have_frame == true){
      break;
    }
  }
  return have_frame;
}




int ltr_int_read_blobs_tir(struct bloblist_type *blt, int min, int max, image *img, tir_info *info)
{
  assert(blt != NULL);
  assert(img != NULL);
  device = info->dev_type;
  p_img = img;
  static size_t size = 0;
  static size_t ptr = 0;
  bool have_frame = false;
  while(1){
    if(ptr >= size){
      ptr = 0;
      if(!ltr_int_receive_data(ltr_int_data_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &size, 1000)){
	ltr_int_log_message("Problem reading data from USB!\n");
        return -1;
      }
    }
    if((have_frame = process_packet(ltr_int_packet, &ptr, size)) == true){
      break;
    }
  }
  
  if(have_frame){
    int res = ltr_int_stripes_to_blobs(3, blt, min, max, img);
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

