#ifndef COM_PROC__H
#define COM_PROC__H

#include <stdbool.h>

typedef enum {STOP, SLEEP, WAKEUP} command_t;

typedef struct{
  float x, y;
  int score;
} blobs_t;

struct blob_type;
command_t getCommand();
void setCommand(command_t cmd);
int getThreshold();
void setThreshold(int thr);
int getMinBlob();
void setMinBlob(int pix);
int getMaxBlob();
void setMaxBlob(int pix);
void setBlobs(struct blob_type *b, int num_blobs);
bool haveNewBlobs();
int getBlobs(struct blob_type * b);
unsigned char* getFramePtr();
bool getFrameFlag();
void setFrameFlag();
void resetFrameFlag();
void printCmd(char *prefix, command_t cmd);
void setWiiIndication(int new_ind);
int getWiiIndication();

#endif
