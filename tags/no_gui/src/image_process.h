#ifndef IMAGE_PROCESS__H
#define IMAGE_PROCESS__H

#include "cal.h"

int search_for_blobs(unsigned char *buf, int w, int h,
                     struct bloblist_type *blobs, int min, int max);


#endif
