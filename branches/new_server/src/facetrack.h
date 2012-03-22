#ifndef FACEDETECT__H
#define FACEDETECT__H

#ifdef __cplusplus
extern "C" {
#endif

#include "image_process.h"

void face_detect(image *img, struct bloblist_type *blt);
void stop_detect();

#ifdef __cplusplus
}
#endif

#endif
