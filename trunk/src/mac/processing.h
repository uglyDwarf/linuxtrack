#ifndef PROCESSING__H
#define PROCESSING__H

#include <stdbool.h>
#include "ipc_utils.h"

bool startProcessing(int w, int h, int buffers, struct mmap_s *mmm_p);
void endProcessing();
bool newFrame(unsigned char *ptr);


#endif