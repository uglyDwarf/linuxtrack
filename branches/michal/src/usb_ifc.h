#ifndef USB_IFACE__H
#define USB_IFACE__H

#include <stdbool.h>
#include <stdlib.h>

typedef enum {NONE = 0, TIR4, TIR5} dev_found;

#ifndef USB_IMPL_ONLY
typedef bool (*init_usb_fun)();
typedef dev_found (*find_tir_fun)();
typedef bool (*prepare_device_fun)(unsigned int config, unsigned int interface, 
  unsigned int in_ep, unsigned int out_ep);
typedef bool (*send_data_fun)(unsigned char data[], size_t size);
typedef bool (*receive_data_fun)(unsigned char data[], size_t size, size_t *transferred,
                         unsigned int timeout);
typedef void (*finish_usb_fun)(unsigned int interface);

extern init_usb_fun init_usb;
extern find_tir_fun find_tir;
extern prepare_device_fun prepare_device;
extern send_data_fun send_data;
extern receive_data_fun receive_data;
extern finish_usb_fun finish_usb;
#endif



#endif
