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

comm_struct *cs = NULL;
off_t size = -1;


command_t getCommand()
{
  lockSemaphore(sem);
  command_t tmp = cs->command;
  unlockSemaphore(sem);
  return tmp;
}

void setCommand(command_t cmd)
{
  lockSemaphore(sem);
  cs->command = cmd;
  unlockSemaphore(sem);
}

int getThreshold()
{
  lockSemaphore(sem);
  int tmp = cs->threshold;
  unlockSemaphore(sem);
  return tmp;
}

void setThreshold(int thr)
{
  lockSemaphore(sem);
  cs->threshold = thr;
  unlockSemaphore(sem);
}

int getMinBlob()
{
  lockSemaphore(sem);
  int tmp = cs->min_blob;
  unlockSemaphore(sem);
  return tmp;
}

void setMinBlob(int pix)
{
  lockSemaphore(sem);
  cs->min_blob = pix;
  unlockSemaphore(sem);
}

int getMaxBlob()
{
  lockSemaphore(sem);
  int tmp = cs->max_blob;
  unlockSemaphore(sem);
  return tmp;
}

void setMaxBlob(int pix)
{
  lockSemaphore(sem);
  cs->max_blob = pix;
  unlockSemaphore(sem);
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

void setBlobs(struct blob_type *b, int num_blobs)
{
  int i;
  lockSemaphore(sem);
  for(i = 0; i < 3; ++i){
    (cs->blobs[i]).x = b[i].x;
    (cs->blobs[i]).y = b[i].y;
    (cs->blobs[i]).score = b[i].score;
  }
  cs->num_blobs = num_blobs;
  ++(cs->frame_counter);
  unlockSemaphore(sem);
}

static int last_val = 0;
bool haveNewBlobs()
{
  if(cs->frame_counter != last_val){
    last_val = cs->frame_counter;
    return true;
  }
  return false;
}

int getBlobs(struct blob_type *b)
{
  int i;
  lockSemaphore(sem);
  for(i = 0; i < 3; ++i){
    b[i].x = (cs->blobs[i]).x;
    b[i].y = (cs->blobs[i]).y;
    b[i].score = (cs->blobs[i]).score;
  }
  last_val = cs->frame_counter;
  i = cs->num_blobs;
  unlockSemaphore(sem);
  return i;
}

unsigned char* getFramePtr()
{
  return &(cs->frame);
}

bool getFrameFlag()
{
  return cs->frame_filled;
}

void setFrameFlag()
{
  cs->frame_filled = true;
}

void resetFrameFlag()
{
  cs->frame_filled = false;
}

void setWiiIndication(int new_ind)
{
  lockSemaphore(sem);
  cs->wii_indication = new_ind;
  unlockSemaphore(sem);
}

int getWiiIndication()
{
  lockSemaphore(sem);
  int res = cs->wii_indication;
  unlockSemaphore(sem);
  return res;
}


