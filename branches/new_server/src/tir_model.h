#ifndef TIR_MODEL__H
#define TIR_MODEL__H

#include "usb_ifc.h"


#ifdef __cplusplus
extern "C" {
#endif
  
  void init_model(char fname[]);
  void fakeusb_send(unsigned char data[], size_t length);
  void fakeusb_receive(unsigned char data[], size_t length, size_t *read, int timeout);
  void close_model();
  
#ifdef __cplusplus
}
#endif

#endif