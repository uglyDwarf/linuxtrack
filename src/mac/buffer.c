#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "buffer.h"

typedef struct{
  unsigned char *ptr;
  bool filled;
}buffer_t;

static int buffer_max = -1;
static buffer_t *buffers = NULL;

bool createBuffers(int count, size_t buf_size)
{
  int i;
  //setup buffer array of given size
  buffers = (buffer_t*)malloc(count * sizeof(buffer_t));
  if(buffers != NULL){
    buffer_max = count - 1;
  }else{
    buffer_max = -1;
    return false;
  }
  
  //Now create buffers
  for(i = 0; i < count; ++i){
    if(((buffers[i]).ptr = malloc(buf_size)) == NULL){
      freeBuffers();
      return false;
    }
    (buffers[i]).filled = false;
  }
  return true;
}

unsigned char *getCurrentBuffer(int i)
{
  //for out of range index assume buffer0 
  if((i < 0) || (i > buffer_max)){
    return buffers[0].ptr;
  }
  return buffers[i].ptr;
}

bool isEmpty(int i)
{
  //for out of range index assume buffer0 
  if((i < 0) || (i > buffer_max)){
    return !buffers[0].filled;
  }
  return !buffers[i].filled;
}

void bufferWritten(int *i)
{
  //for out of range index assume buffer0 
  if((*i < 0) || (*i > buffer_max)){
    *i = 0;
  }
  buffers[*i].filled = true;

  ++(*i);
  if(*i > buffer_max){
    *i = 0;
  }
}

void bufferRead(int *i)
{
  //for out of range index assume buffer0 
  if((*i < 0) || (*i > buffer_max)){
    *i = 0;
  }
  buffers[*i].filled = false;

  ++(*i);
  if(*i > buffer_max){
    *i = 0;
  }
}

void freeBuffers()
{
  int i;
  for(i = 0; i <= buffer_max; ++i){
    if(buffers[i].ptr != NULL){
      free(buffers[i].ptr);
      buffers[i].ptr = NULL;
    }
  }
  free(buffers);
  buffers = NULL;
  buffer_max = -1;
}

void printBuffers()
{
  int i;
  for(i = 0; i <= buffer_max; ++i){
    printf("Buffer %d (%p) - %s\n", i, buffers[i].ptr, 
	     buffers[i].filled ? "FULL": "EMPTY");
  }
}

int r = -1;
int w = 0;
  
void read()
{
  if(!isEmpty(r)){
    printf("Processing buffer %d @ %p\n", r, getCurrentBuffer(r));
    bufferRead(&r);
  }else{
    printf("No new buffer!\n");
  }
}

void write()
{
  if(isEmpty(w)){
    printf("Writing buffer %d @ %p\n", w, getCurrentBuffer(w));
    bufferWritten(&w);
  }else{
    printf("No empty buffer!\n");
  }
}

