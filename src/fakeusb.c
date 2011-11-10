#include <stdbool.h>
#include <stdio.h>
#define USB_IMPL_ONLY
#include "usb_ifc.h"
#include "utils.h"

bool ltr_int_init_usb()
{
  return true;
}

dev_found ltr_int_find_tir(unsigned int devid)
{
  (void) devid;
  return TIR3;
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
  if((data[0] == 17) && (data[1] == 01)){
    send_cfg = true;
  }
  return true;
}

unsigned char packet[] = {0x06, 0x1c, 0x00, 0x00, 0x00, 0x00};
unsigned char cfg[] = {0x09, 0x40, 0x03, 0x01, 0x00, 0x75, 0x4a, 0x04, 0x00};

bool ltr_int_receive_data(unsigned char data[], size_t size, size_t *transferred,
                  unsigned int timeout)
{
  (void) timeout;
  unsigned int i;
  //size_t len;
  if(send_cfg){
    for(i = 0; i < cfg[0]; ++i){
      data[i] = cfg[i];
    }
    *transferred = cfg[0];
  }else{
    for(i = 0; i < packet[0]; ++i){
      data[i] = packet[i];
    }
    *transferred = packet[0];
  }
  usleep(10000);
  return true;
}

void ltr_int_finish_usb(unsigned int interface)
{
  (void) interface;
}


