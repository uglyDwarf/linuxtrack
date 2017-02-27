#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#define USB_IMPL_ONLY
#include "usb_ifc.h"
#include "utils.h"
#include <string.h>
#include "tir_model.h"

#define PKT_MAX 16384

  int current_model = 0;
  char *data_file = NULL;

static int get_tir_type()
{
  char *fname = getenv("LINUXTRACK_STIMULI");
  data_file = fname;
  if(fname == NULL){
    return TIR2;
  }
  char *dot = strrchr(fname, '.');
  if(dot == NULL){
    return TIR2;
  }
  //skip the dot
  dot += 1;
  if(strcmp("tir5v3", dot) == 0){
    return TIR5V3;
  }else if(strcmp("tir5", dot) == 0){
    return TIR5;
  }else if(strcmp("tir4", dot) == 0){
    return TIR4;
  }else if(strcmp("tir3", dot) == 0){
    return TIR3;
  }else if(strcmp("sn3", dot) == 0){
    return SMARTNAV3;
  }else if(strcmp("sn4", dot) == 0){
    return SMARTNAV4;
  }
  //fallback
  return TIR2;
}

static char pkt_dir[10];
static int read_packet(uint8_t buffer[], int length)
{
  int len;
  int ptr;
  int buf_ptr = 0;
  int num;
  char line[PKT_MAX * 5];
  static FILE *f = NULL;
  static bool check_file = true;
  
  if(check_file){
    check_file = false;
    char *fname = getenv("LINUXTRACK_STIMULI");
    if(fname == NULL){
      return 0;
    }
    f = fopen(fname, "r");
  }
  if(f == NULL){
    return 0;
  }
  char *res;
 repeat:
  res = fgets(line, sizeof(line), f);
  if(res == NULL){
    fseek(f, 0L, SEEK_SET);
    res = fgets(line, sizeof(line), f);
  }
  
  if(res != NULL){
    ptr = 0;
    //printf("%s\n", line);
    
    sscanf(&(line[0]), "%8s%n", pkt_dir, &len);
    ptr += len;
    if(pkt_dir[0] != 'i'){
      goto repeat;
    }
    while(sscanf(&(line[ptr]), "%X%n", &num, &len) > 0){
      ptr += len;
      buffer[buf_ptr++] = num;
      if(buf_ptr == length){
        break;
      }
    }
    if(ptr == 0) goto repeat;
  }else{
    return 0;
  }
  return buf_ptr;
}

bool ltr_int_init_usb()
{
  printf("Initializing fakeusb!\n");
  current_model = get_tir_type();
  switch(current_model){
    case SMARTNAV3:
    case SMARTNAV4:
    case TIR4:
    case TIR5:
    case TIR5V3:
      return init_model(data_file, current_model);
      break;
    default:
      break;
  }
  return true;
}

dev_found ltr_int_find_tir(void)
{
  printf("Looking for the TrackIr\n");
  return get_tir_type();
}

bool ltr_int_prepare_device(unsigned int config, unsigned int interface)
{
  (void) config;
  (void) interface;
  return true;
}

static bool send_cfg = false;

bool ltr_int_send_data(int out_ep, unsigned char data[], size_t size)
{
  (void) out_ep;
  switch(current_model){
    case SMARTNAV3:
    case SMARTNAV4:
    case TIR4:
    case TIR5:
    case TIR5V3:
      fakeusb_send(out_ep, data, size);
      return true;
    default:
      break;
  }
  unsigned int i; 
  printf("EP%02X:", out_ep);
  for(i = 0; i <size; ++i){
    printf("%02X ", data[i]);
  }
  printf("\n");
  if(data[0] == 17){
    send_cfg = true;
  }
  return true;
}

//static unsigned char packet[] = {0x05, 0x1c, 0x00, 0x00, 0x00}; 
static unsigned char cfg[] = {0x09, 0x40, 0x03, 0x00, 0x00, 0x34, 0x5d, 0x03, 0x00};
static uint8_t pkt_buf[PKT_MAX];

//static size_t data_len(size_t s1, size_t s2)
//{
//  return (s1 < s2) ? s1 : s2;
//}

bool ltr_int_receive_data(int in_ep, unsigned char data[], size_t size, size_t *transferred,
                  long timeout)
{
  (void) in_ep;
  switch(current_model){
    case SMARTNAV3:
    case SMARTNAV4:
    case TIR4:
    case TIR5:
    case TIR5V3:
      fakeusb_receive(in_ep, data, size, transferred, timeout);
      ltr_int_usleep(8000);
      return true;
      break;
    default:
      break;
  }

  (void) timeout;
  size_t i;
  
  if(send_cfg){
    for(i = 0; i < size; ++i){
      data[i] = cfg[i];
    }
    *transferred = size;
  }else{
    uint8_t *pkt_data;
    int plen = read_packet(pkt_buf, sizeof(pkt_buf));
    pkt_data = pkt_buf;
    *transferred = plen;
    for(i = 0; i < *transferred; ++i){
      data[i] = pkt_data[i];
    }
  }
  ltr_int_usleep(10000);
  return true;
}

void ltr_int_finish_usb(unsigned int interface)
{
  (void) interface;
  switch(current_model){
    case SMARTNAV3:
    case SMARTNAV4:
    case TIR4:
    case TIR5:
    case TIR5V3:
      close_model();
      break;
    default:
      break;
  }
}






