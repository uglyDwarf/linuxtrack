#ifndef COM_PROC__H
#define COM_PROC__H

#include <stdbool.h>
#include <ipc_utils.h>

typedef enum {STOP, SLEEP, WAKEUP} command_t;

typedef struct{
  float x, y;
  int score;
} blobs_t;

struct blob_type;

command_t getCommand(struct mmap_s *mmm);
void setCommand(struct mmap_s *mmm, command_t cmd);
int getThreshold(struct mmap_s *mmm);
void setThreshold(struct mmap_s *mmm, int thr);
int getMinBlob(struct mmap_s *mmm);
void setMinBlob(struct mmap_s *mmm, int pix);
int getMaxBlob(struct mmap_s *mmm);
void setMaxBlob(struct mmap_s *mmm, int pix);
void setBlobs(struct mmap_s *mmm, struct blob_type *b, int num_blobs);
bool haveNewBlobs(struct mmap_s *mmm);
int getBlobs(struct mmap_s *mmm, struct blob_type * b);
unsigned char* getFramePtr(struct mmap_s *mmm);
bool getFrameFlag(struct mmap_s *mmm);
void setFrameFlag(struct mmap_s *mmm);
void resetFrameFlag(struct mmap_s *mmm);
void printCmd(char *prefix, command_t cmd);
void setWiiIndication(struct mmap_s *mmm, int new_ind);
int getWiiIndication(struct mmap_s *mmm);
int get_com_size();
#endif
