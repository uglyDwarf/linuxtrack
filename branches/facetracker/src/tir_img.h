#ifndef TIR_IMG__H
#define TIR_IMG__H

#include "list.h"
#include "image_process.h"
#include "tir_hw.h"

int ltr_int_read_blobs_tir(struct bloblist_type *blt, int min, int max, image *img, tir_info *info);

#endif
