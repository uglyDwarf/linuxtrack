#ifndef USB_IFACE__H
#define USB_IFACE__H

#include <stdbool.h>
#include <stdlib.h>

typedef enum {NONE = 0, TIR4, TIR5} dev_found;

extern bool init_usb();
extern dev_found find_tir();
extern bool prepare_device(unsigned int config, unsigned int interface, 
  unsigned int in_ep, unsigned int out_ep);
extern bool send_data(unsigned char data[], size_t size);
extern bool receive_data(unsigned char data[], size_t size, size_t *transferred);
extern void finish_usb(unsigned int interface);
#endif
