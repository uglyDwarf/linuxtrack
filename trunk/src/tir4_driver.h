#ifndef TIR4_DRIVER__H
#define TIR4_DRIVER__H

#include <usb.h>

void init(void);
void shutdown(void);
void set_configuration_tir4();
void set_altinterface_tir4(void);
void claim_tir4(void);
void release_tir4(void);
void close_tir4(void);
struct usb_device *find_device(uint16_t, uint16_t);
void error_alert(char *);

#endif
