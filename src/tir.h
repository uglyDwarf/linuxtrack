#ifndef TIR__H
#define TIR__H

#include <stdbool.h>

extern bool open_tir();
extern bool pause_tir();
extern bool resume_tir();
extern bool read_frame();
extern bool close_tir(); 

#endif
