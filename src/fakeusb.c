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
  return TIR2;
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

unsigned char packet[] = {0x3e, 0x1c, 0xa7, 0x79, 0x83, 
                                      0xa8, 0x7b, 0x80, 
                                      0xa9, 0x74, 0x75, 0xa9, 0x89, 0x8b, 
                                      0xab, 0x72, 0x73, 0xab, 0x7a, 0x7b, 0xab, 0x81, 0x83, 
                                      0xac, 0x7d, 0x81, 
                                      0xad, 0x7c, 0x7f, 
                                      0xae, 0x80, 0x83, 
                                      0xaf, 0x82, 0x85, 
                                      0xb2, 0x6d, 0x6f, 
                                      0xb5, 0x7a, 0x7b, 
                                      0xb6, 0x57, 0x59, 
                                      0xb7, 0x6e, 0x71, 
                                      0xb7, 0x8d, 0x8e, 
                                      0xb8, 0x72, 0x7b, 0xb8, 0x81, 0x82, 
                                      0xba, 0x76, 0x77, 0xba, 0x7d, 0x7e, 
                                      0xfe, 0xcd};
//unsigned char packet[] = {0x11, 0x1c, 0x00, 0x00, 0x00, 0xc5, 0xc6, 0xc8, 0xc6, 0xc5, 0xca, 0xc7, 
//                          0xc5, 0xca, 0xc8, 0xc5, 0xc8};
unsigned char cfg[] = {0x09, 0x40, 0x03, 0x00, 0x00, 0x34, 0x5d, 0x03, 0x00};

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


