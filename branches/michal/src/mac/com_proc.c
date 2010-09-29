#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
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
bool initialized = false;
char *fname_dup = NULL;

struct semaphore_t{
  int fd;
}semaphore_t;

semaphore_p sem = NULL;

semaphore_p semaphoreFromFd(int fd)
{
  semaphore_p res = (semaphore_p)ltr_int_my_malloc(sizeof(semaphore_t));
  res->fd = fd;
  return res;
}

semaphore_p createSemaphore(char *fname)
{
  if(fname == NULL){
    return NULL;
  }
  int fd = open(fname, O_RDWR | O_CREAT, 0600);
  if(fd == -1){
    perror("open: ");
    return NULL;
  }
  return semaphoreFromFd(fd);
}

bool lockSemaphore(semaphore_p semaphore)
{
  if(semaphore == NULL){
    return false;
  }
  int res = lockf(semaphore->fd, F_TLOCK,0);
  if(res == 0){
    return true;
  }else{
    perror("lockf: ");
    return false;
  }
}

bool unlockSemaphore(semaphore_p semaphore)
{
  if(semaphore == NULL){
    return false;
  }
  int res = lockf(semaphore->fd, F_ULOCK,0);
  if(res == 0){
    return true;
  }else{
    return false;
  }
}

void closeSemaphore(semaphore_p semaphore)
{
  close(semaphore->fd);
  free(semaphore);
}

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



bool mmap_file(const char *fname, size_t tmp_size)
{
  tmp_size += sizeof(comm_struct);
  if(initialized){
    return false;
  }
  fname_dup = strdup(fname);
  int fd = open(fname, O_RDWR | O_CREAT | O_NOFOLLOW, 0700);
  if(fd < 0){
    perror("open: ");
    return false;
  }
  size = tmp_size;
  
  //Check if file does have needed length...
  struct stat file_stat;
  bool truncate = true;
  if(fstat(fd, &file_stat) == 0){
    if(file_stat.st_size == size){
      truncate = false;
    }
  }
  
  if(truncate){
    int res = ftruncate(fd, size);
    if (res == -1) {
      perror("ftruncate: ");
      close(fd);
      return false;
    }
  }
  cs = (comm_struct*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
		   fd, 0);
  if(cs == (comm_struct*)-1){
    perror("mmap: ");
    close(fd);
    return false;
  }
//  close(fd);
  sem = semaphoreFromFd(fd);
  sem->fd = fd;
  cs->frame_counter = 0;
  initialized = true;
  return cs != NULL;
}

bool unmap_file()
{
  if(cs == NULL){
    return true;
  }
  closeSemaphore(sem);
  if(fname_dup != NULL){
    free(fname_dup);
    fname_dup = NULL;
  }
  int res = munmap(cs, size);
  if(res < 0){
    perror("munmap: ");
  }
  initialized = false;
  return res == 0;
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


