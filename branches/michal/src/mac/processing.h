#ifndef PROCESSING__H
#define PROCESSING__H

#include <stdbool.h>

bool startProcessing(int w, int h, int buffers);
void endProcessing();
bool newFrame(unsigned char *ptr);


#endif