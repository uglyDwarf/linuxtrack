#ifndef BUFFER__H
#define BUFFER__H

#include <stdbool.h>

bool createBuffers(int buffers, size_t buf_size);
unsigned char *getCurrentBuffer(int i);
bool isEmpty(int i);
void bufferWritten(int *i);
void bufferRead(int *i);
void freeBuffers();

#endif