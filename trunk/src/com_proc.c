#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/file.h>
#include <string.h>
#include <com_proc.h>
#include <cal.h>
#include <utils.h>
#include <ipc_utils.h>

typedef struct{
  command_t        command;
  int              threshold;
  int              min_blob;
  int              max_blob;
  float            exp_filt_factor;
  int              opt_level;
  int              wii_indication;
  bool             frame_filled;
  int              frame_counter;
  int              num_blobs;
  struct blob_type blobs[MAX_BLOBS];
  unsigned char    frame;
} comm_struct;

//static off_t size = -1;


command_t ltr_int_getCommand(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  command_t tmp = cs->command;
  ltr_int_unlockSemaphore(mmm->sem);
  return tmp;
}

void ltr_int_setCommand(struct mmap_s *mmm, command_t cmd)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  cs->command = cmd;
  ltr_int_unlockSemaphore(mmm->sem);
}

int ltr_int_getThreshold(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  int tmp = cs->threshold;
  ltr_int_unlockSemaphore(mmm->sem);
  return tmp;
}

void ltr_int_setThreshold(struct mmap_s *mmm, int thr)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  cs->threshold = thr;
  ltr_int_unlockSemaphore(mmm->sem);
}

int ltr_int_getMinBlob(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  int tmp = cs->min_blob;
  ltr_int_unlockSemaphore(mmm->sem);
  return tmp;
}

void ltr_int_setMinBlob(struct mmap_s *mmm, int pix)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  cs->min_blob = pix;
  ltr_int_unlockSemaphore(mmm->sem);
}

int ltr_int_getMaxBlob(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  int tmp = cs->max_blob;
  ltr_int_unlockSemaphore(mmm->sem);
  return tmp;
}

void ltr_int_setMaxBlob(struct mmap_s *mmm, int pix)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  cs->max_blob = pix;
  ltr_int_unlockSemaphore(mmm->sem);
}



void ltr_int_printCmd(char *prefix, command_t cmd)
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

void ltr_int_setBlobs(struct mmap_s *mmm, struct blob_type *b, int num_blobs)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  int i;
  ltr_int_lockSemaphore(mmm->sem);
  int blobs = (num_blobs < MAX_BLOBS) ? num_blobs : MAX_BLOBS;
  for(i = 0; i < blobs; ++i){
    (cs->blobs[i]).x = b[i].x;
    (cs->blobs[i]).y = b[i].y;
    (cs->blobs[i]).score = b[i].score;
  }
  cs->num_blobs = blobs;
  ++(cs->frame_counter);
  ltr_int_unlockSemaphore(mmm->sem);
}

static int last_val = 0;
bool ltr_int_haveNewBlobs(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  if(cs->frame_counter != last_val){
    last_val = cs->frame_counter;
    return true;
  }
  return false;
}

int ltr_int_getBlobs(struct mmap_s *mmm, struct blob_type *b, int num_blobs)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  int i;
  ltr_int_lockSemaphore(mmm->sem);
  int blobs = (cs->num_blobs > num_blobs) ? num_blobs : cs->num_blobs;
  for(i = 0; i < blobs; ++i){
    b[i].x = (cs->blobs[i]).x;
    b[i].y = (cs->blobs[i]).y;
    b[i].score = (cs->blobs[i]).score;
  }
  last_val = cs->frame_counter;
  ltr_int_unlockSemaphore(mmm->sem);
  return cs->num_blobs;
}

unsigned char* ltr_int_getFramePtr(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  return &(cs->frame);
}

bool ltr_int_getFrameFlag(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  return cs->frame_filled;
}

void ltr_int_setFrameFlag(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  cs->frame_filled = true;
}

void ltr_int_resetFrameFlag(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  cs->frame_filled = false;
}

void ltr_int_setWiiIndication(struct mmap_s *mmm, int new_ind)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  cs->wii_indication = new_ind;
  ltr_int_unlockSemaphore(mmm->sem);
}

int ltr_int_getWiiIndication(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  int res = cs->wii_indication;
  ltr_int_unlockSemaphore(mmm->sem);
  return res;
}

int ltr_int_get_com_size()
{
  return sizeof(comm_struct);
}

int ltr_int_getOptLevel(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  int res = cs->opt_level;
  ltr_int_unlockSemaphore(mmm->sem);
  return res;
}

void ltr_int_setOptLevel(struct mmap_s *mmm, int new_opt)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  cs->opt_level = new_opt;
  ltr_int_unlockSemaphore(mmm->sem);
}

float ltr_int_getEff(struct mmap_s *mmm)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  float res = cs->exp_filt_factor;
  ltr_int_unlockSemaphore(mmm->sem);
  return res;
}

void ltr_int_setEff(struct mmap_s *mmm, float new_eff)
{
  comm_struct *cs = (comm_struct*)mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  cs->exp_filt_factor = new_eff;
  ltr_int_unlockSemaphore(mmm->sem);
}

