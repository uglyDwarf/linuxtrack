#ifndef SN4_COM__H
#define SN4_COM__H

#include <stdint.h>
#include <sys/time.h>

typedef struct{
  uint8_t btns;
  struct timeval timestamp;
} sn4_btn_event_t;

#endif
