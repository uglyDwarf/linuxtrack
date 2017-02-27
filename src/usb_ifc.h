#ifndef USB_IFACE__H
#define USB_IFACE__H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum {NOT_TIR = 0, TIR2, TIR3, TIR4, TIR5, TIR5V2, SMARTNAV3, SMARTNAV4, TIR5V3, PS3EYE,
              NOT_PERMITTED = 16384} dev_found;

typedef bool (init_usb_fun)(void);
typedef dev_found (find_tir_fun)(void);
typedef bool (find_p3e_fun)(void);
typedef bool (prepare_device_fun)(unsigned int config, unsigned int interface);
typedef bool (send_data_fun)(int out_ep, unsigned char data[], size_t size);
typedef bool (receive_data_fun)(int in_ep, unsigned char data[], size_t size, size_t *transferred,
                         long timeout);
typedef void (finish_usb_fun)(unsigned int interface);
typedef bool (ctrl_data_fun)(uint8_t req_type, uint8_t req, uint16_t val, uint16_t index,
                            unsigned char data[], size_t size);


#ifndef USB_IMPL_ONLY
extern init_usb_fun *ltr_int_init_usb;
extern find_tir_fun *ltr_int_find_tir;
extern find_p3e_fun *ltr_int_find_p3e;
extern prepare_device_fun *ltr_int_prepare_device;
extern send_data_fun *ltr_int_send_data;
extern receive_data_fun *ltr_int_receive_data;
extern ctrl_data_fun *ltr_int_ctrl_data;
extern finish_usb_fun *ltr_int_finish_usb;
#else
/*
bool ltr_int_init_usb();
dev_found ltr_int_find_tir();
bool ltr_int_prepare_device(unsigned int config, unsigned int interface);
bool ltr_int_send_data(int out_ep, unsigned char data[], size_t size);
bool ltr_int_receive_data(int in_ep, unsigned char data[], size_t size, size_t *transferred,
                         unsigned int timeout);
void ltr_int_finish_usb(unsigned int interface);
*/
extern init_usb_fun ltr_int_init_usb;
extern find_p3e_fun ltr_int_find_p3e;
extern find_tir_fun ltr_int_find_tir;
extern prepare_device_fun ltr_int_prepare_device;
extern send_data_fun ltr_int_send_data;
extern receive_data_fun ltr_int_receive_data;
extern ctrl_data_fun ltr_int_ctrl_data;
extern finish_usb_fun ltr_int_finish_usb;


#endif



#endif
