#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#define USB_IMPL_ONLY
#include "usb_ifc.h"
#include "utils.h"
#include <string.h>

#define PKT_MAX 16384

static int get_tir_type()
{
  char *fname = getenv("LINUXTRACK_STIMULI");
  if(fname == NULL){
    return TIR2;
  }
  int last = strlen(fname);
  if(last == 0){
    return TIR2;
  }
  switch(fname[last-1]){
    case '2': 
      return TIR2;
      break;
    case '3': 
      return TIR3;
      break;
    case '4': 
      return TIR4;
      break;
    case '5': 
      return TIR5;
      break;
    default:
      return TIR2;
      break;
  }
}


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
  return true;
}

dev_found ltr_int_find_tir(unsigned int devid)
{
  (void) devid;
  return get_tir_type();
}

bool ltr_int_prepare_device(unsigned int config, unsigned int interface, 
  unsigned int in_ep, unsigned int out_ep)
{
  (void) config;
  (void) interface;
  (void) in_ep;
  (void) out_ep;
  return true;
}

bool send_cfg = false;

bool ltr_int_send_data(unsigned char data[], size_t size)
{
  unsigned int i;
  for(i = 0; i <size; ++i){
    printf("%02X ", data[i]);
  }
  printf("\n");
  if(data[0] == 17){
    send_cfg = true;
  }
  return true;
}

unsigned char packet[] = {0x05, 0x1c, 0x00, 0x00, 0x00}; 
unsigned char cfg[] = {0x09, 0x40, 0x03, 0x00, 0x00, 0x34, 0x5d, 0x03, 0x00};
uint8_t pkt_buf[PKT_MAX];

static size_t data_len(size_t s1, size_t s2)
{
  return (s1 < s2) ? s1 : s2;
}

bool ltr_int_receive_data(unsigned char data[], size_t size, size_t *transferred,
                  unsigned int timeout)
{
  (void) timeout;
  size_t i;
  
  if(send_cfg){
    for(i = 0; i < data_len(cfg[0], size); ++i){
      data[i] = cfg[i];
    }
    *transferred = cfg[0];
  }else{
    uint8_t *pkt_data;
    int plen = read_packet(pkt_buf, sizeof(pkt_buf));
    if(plen > 0){
      pkt_data = pkt_buf;
    }else{
      pkt_data = packet;
    }
    for(i = 0; i < data_len(pkt_data[0], size); ++i){
      data[i] = pkt_data[i];
    }
    *transferred = pkt_data[0];
  }
  ltr_int_usleep(10000);
  return true;
}

void ltr_int_finish_usb(unsigned int interface)
{
  (void) interface;
}


