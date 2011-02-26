#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/file.h>
#include <string.h>
#include "com_proc.h"
#include "../cal.h"
#include "../utils.h"
#include "../ipc_utils.h"

typedef struct{
  command_t        command;
  int              threshold;
  int              min_blob;
  int              max_blob;
  int              wii_indication;
  bool             frame_filled;
  int              frame_counter;
  int              num_blobs;
  struct blob_type blobs[3];
  unsigned char    frame;
} comm_struct;

off_t size = -1;


command_t getCommand(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  command_t tmp = cs->command;
  ltr_int_unlockSemaphore(mmm->sem);
  return tmp;
}

void setCommand(struct mmap_s *mmm, command_t cmd)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  cs->command = cmd;
  ltr_int_unlockSemaphore(mmm->sem);
}

int getThreshold(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  int tmp = cs->threshold;
  ltr_int_unlockSemaphore(mmm->sem);
  return tmp;
}

void setThreshold(struct mmap_s *mmm, int thr)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  cs->threshold = thr;
  ltr_int_unlockSemaphore(mmm->sem);
}

int getMinBlob(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  int tmp = cs->min_blob;
  ltr_int_unlockSemaphore(mmm->sem);
  return tmp;
}

void setMinBlob(struct mmap_s *mmm, int pix)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  cs->min_blob = pix;
  ltr_int_unlockSemaphore(mmm->sem);
}

int getMaxBlob(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  int tmp = cs->max_blob;
  ltr_int_unlockSemaphore(mmm->sem);
  return tmp;
}

void setMaxBlob(struct mmap_s *mmm, int pix)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  cs->max_blob = pix;
  ltr_int_unlockSemaphore(mmm->sem);
}



void printCmd(char *prefix, command_t cmd)
{
  switch(cmd){
    case STOP:
      printf("%s cmd: STOP\n", prefix);
      break;
    case SLEEP:
      printf("%s cmd: SLEEP\n", prefix);
      break;
    case WAKEUP:
      printf("%s cmd: WAKEUP\n", prefix);
      break;
  }
}

void setBlobs(struct mmap_s *mmm, struct blob_type *b, int num_blobs)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  int i;
  ltr_int_lockSemaphore(mmm->sem);
  for(i = 0; i < 3; ++i){
    (cs->blobs[i]).x = b[i].x;
    (cs->blobs[i]).y = b[i].y;
    (cs->blobs[i]).score = b[i].score;
  }
  cs->num_blobs = num_blobs;
  ++(cs->frame_counter);
  ltr_int_unlockSemaphore(mmm->sem);
}

static int last_val = 0;
bool haveNewBlobs(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  if(cs->frame_counter != last_val){
    last_val = cs->frame_counter;
    return true;
  }
  return false;
}

int getBlobs(struct mmap_s *mmm, struct blob_type *b)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  int i;
  ltr_int_lockSemaphore(mmm->sem);
  for(i = 0; i < 3; ++i){
    b[i].x = (cs->blobs[i]).x;
    b[i].y = (cs->blobs[i]).y;
    b[i].score = (cs->blobs[i]).score;
  }
  last_val = cs->frame_counter;
  i = cs->num_blobs;
  ltr_int_unlockSemaphore(mmm->sem);
  return i;
}

unsigned char* getFramePtr(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  return &(cs->frame);
}

bool getFrameFlag(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  return cs->frame_filled;
}

void setFrameFlag(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  cs->frame_filled = true;
}

void resetFrameFlag(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  cs->frame_filled = false;
}

void setWiiIndication(struct mmap_s *mmm, int new_ind)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  cs->wii_indication = new_ind;
  ltr_int_unlockSemaphore(mmm->sem);
}

int getWiiIndication(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  int res = cs->wii_indication;
  ltr_int_unlockSemaphore(mmm->sem);
  return res;
}

int get_com_size()
{
  return sizeof(comm_struct);
}

