#ifndef BUFFERING__H
#define BUFFERING__H

#include <QMutex>
#include <QVector>
#include <QRgb>

class QImage;

class buffer{
 public:
  buffer();
  ~buffer();
  bool resizeBuffer(int width, int height);
  unsigned char *getBuffer(){return buf;};
  QImage *getImage(){return img;};
  void clearBuffer();
 private:
  unsigned char *buf;
  QImage *img;
  int w, h;
  static QVector<QRgb> *palette;
  static void initPalette();
};


class buffering{
 public:
  buffering(int n = 3);
  ~buffering(){};
  
  void init(){current = writeChecked = readChecked = -1;};
  bool resizeBuffers(int w, int h);
  bool writeBuffer(buffer **wb);
  void bufferWritten();
  bool readBuffer(buffer **rb);
  void bufferRead();
 private:
  int noBuffers;
  std::vector <buffer> buffers;
  int current; //buffer last written
  int writeChecked; //buffer checked for writing
  int readChecked; //buffer checked for reading
  QMutex lock;
};

#endif
