#ifndef TIR_IMG__H
#define TIR_IMG__H

#include "list.h"

typedef struct{
  float x,y;
  float score;
} blob;

int read_blobs_tir(plist *blob_list, unsigned char pic[], unsigned int x, unsigned int y);

#endif