#ifndef COM_PROC__H
#define COM_PROC__H

#include <stdbool.h>

typedef enum {STOP, SLEEP, WAKEUP} command_t;

typedef struct{
  float x, y;
  int score;
} blobs_t;

struct blob_type;
typedef struct semaphore_t *semaphore_p;

semaphore_p createSemaphore(char *fname);
bool lockSemaphore(semaphore_p semaphore);
bool unlockSemaphore(semaphore_p semaphore);
void closeSemaphore(semaphore_p semaphore);

bool mmap_file(const char *fname, size_t tmp_size);
bool unmap_file();
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
#endif
