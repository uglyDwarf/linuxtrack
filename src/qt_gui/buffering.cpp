#include <iostream>
#include <vector>
#include <QImage>
#include <cstring>
#include "buffering.h"

/*
TODO:
the concept of having a bitmap set according to the resolution is flawed - res can change,
and the memory outside allocated would be used...

Possible solution - frame would contain capture flag; if set, the driver would allocate the memory,
and the application would copy its contents...
Pro: corner situation resolved
Con: small performance hit - copying the memory; but only when capturing...
*/

QVector<QRgb> *buffer::palette = NULL;

void buffer::initPalette()
{
  palette = new QVector<QRgb>();
  palette->clear();
  for(int col = 0; col < 256; ++col){
    palette->push_back(qRgb(col, col, col));
  }
}

buffer::buffer(): buf(NULL), img(NULL), w(0), h(0)
{
  if(palette == NULL){
    initPalette();
  }
}

void buffer::clearBuffer()
{
  if(buf != NULL){
    memset(buf, 0, w * h);
  }
}

bool buffer::resizeBuffer(int width, int height)
{
  if((width == w) && (height == h)){
    return true;
  }
  delete img;
  img = NULL;
  free(buf);
  w = width;
  h = height;
  buf = (unsigned char *)malloc(w * h);
  img = new QImage(buf, w, h, w, QImage::Format_Indexed8);
  img->setColorTable(*palette);
  return true;
}

buffer::~buffer()
{
  delete img;
  free(buf);
}

buffering::buffering(int n): noBuffers(n), current(-1), writeChecked(-1), readChecked(-1), lock()
{
  buffers.resize(noBuffers);
}

bool buffering::resizeBuffers(int w, int h)
{
  lock.lock();
  bool res = true;
  for(int i = 0; i < noBuffers; ++i){
    res &= ((buffers[i]).resizeBuffer(w, h));
  }
  lock.unlock();
  return res;
}

bool buffering::writeBuffer(buffer **wb)
{
  lock.lock();
  if(writeChecked != -1){
    lock.unlock();
    return false;
  }
  int tmpIndex;
  for(int i = 1; i < noBuffers; ++i){
    tmpIndex = (current + i) % noBuffers;
    if(tmpIndex != readChecked){
      writeChecked = tmpIndex;
      break;
    }
  }
  //the assumption is, that with 3+ buffers, there must be more than one available
  // (only one can be read locked, no write locked)
  *wb = &(buffers[writeChecked]);
  std::cout<<"Write checked buf "<<writeChecked<<std::endl;
  (*wb)->clearBuffer();
  lock.unlock();
  return true;
}

void buffering::bufferWritten()
{
  lock.lock();
  if(writeChecked == -1){
    lock.unlock();
    return;
  }
  current = writeChecked;
  std::cout<<"Write unchecked buf "<<writeChecked<<std::endl;
  writeChecked = -1;
  lock.unlock();
}

bool buffering::readBuffer(buffer **rb)
{
  lock.lock();
  if((readChecked != -1) || (current == -1)){
    lock.unlock();
    return false;
  }
  readChecked = current;
  *rb = &(buffers[readChecked]);
  std::cout<<"Read checked buf "<<current<<std::endl;
  lock.unlock();
  return true;
}

void buffering::bufferRead()
{
  lock.lock();
  std::cout<<"Read unchecked buf "<<current<<std::endl;
  readChecked = -1;
  lock.unlock();
}

#ifdef TEST_IMPL
#include <cassert>

int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  std::cout<<"Hello World!"<<std::endl;
  
  buffering b(3);
  buffer *buf = NULL;
  assert(!b.readBuffer(&buf));
  buf = NULL;
  assert(b.writeBuffer(&buf) && (buf != NULL));
  assert(!b.writeBuffer(&buf));  
  b.bufferWritten();
  buf = NULL;
  assert(b.readBuffer(&buf) && (buf != NULL));
  b.bufferRead();
  return 0;
}

#endif


