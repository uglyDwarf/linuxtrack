#define __USE_GNU
#define _GNU_SOURCE
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "ipc_utils.h"
#include "utils.h"

static pid_t child;

bool fork_child(char *args[])
{
  if((child = fork()) == 0){
    //Child here
    execv(args[0], args);
    perror("execv");
    return false;
  }
  return true;
}

bool wait_child_exit(int limit)
{
  pid_t res;
  int status;
  int cntr = 0;
  do{
//    printf("Waiting!\n");
    usleep(100000);
    res = waitpid(child, &status, WNOHANG);
    ++cntr;
  }while((res != child) && (cntr < limit));
  if((res == child) && (WIFEXITED(status))){
    return true;
  }else{
    return false;
  }
}

struct semaphore_t{
  int fd;
}semaphore_t;

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
  int res = lockf(semaphore->fd, F_LOCK,0);
  if(res == 0){
    return true;
  }else{
    return false;
  }
}


bool tryLockSemaphore(semaphore_p semaphore)
{
  if(semaphore == NULL){
    return false;
  }
  int res = lockf(semaphore->fd, F_TLOCK,0);
  if(res == 0){
    return true;
  }else{
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


bool mmap_file(const char *fname, size_t tmp_size, struct mmap_s *m)
{
  m->fname = ltr_int_my_strdup(fname);
  int fd = open(fname, O_RDWR | O_CREAT | O_NOFOLLOW, 0700);
  if(fd < 0){
    perror("open: ");
    return false;
  }
  m->size = tmp_size;
  
  //Check if file does have needed length...
  struct stat file_stat;
  bool truncate = true;
  if(fstat(fd, &file_stat) == 0){
    if(file_stat.st_size == (ssize_t)m->size){
      truncate = false;
    }
  }
  
  if(truncate){
    int res = ftruncate(fd, m->size);
    if (res == -1) {
      perror("ftruncate: ");
      close(fd);
      return false;
    }
  }
  m->data = mmap(NULL, m->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(m->data == (void*)-1){
    perror("mmap: ");
    close(fd);
    return false;
  }
//  close(fd);
  m->sem = semaphoreFromFd(fd);
  m->sem->fd = fd;
  return true;
}

bool unmap_file(struct mmap_s *m)
{
  if(m->data == NULL){
    return true;
  }
  closeSemaphore(m->sem);
  if(m->fname != NULL){
    free(m->fname);
    m->fname = NULL;
  }
  int res = munmap(m->data, m->size);
  if(res < 0){
    perror("munmap: ");
  }
  return res == 0;
}

//the fname argument should end with XXXXXX;
//  it is also modified by the call to contain the actual filename.
//
//Returns opened file descriptor
int open_tmp_file(char *fname)
{
  umask(S_IWGRP | S_IWOTH);
  return mkstemp(fname);
}

//Closes and removes the file...
void close_tmp_file(char *fname, int fd)
{
  unlink(fname);
  close(fd);
}

