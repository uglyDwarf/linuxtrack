
#ifndef __USE_GNU
  #define __USE_GNU
#endif
#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>

#ifndef LIBLINUXTRACK_SRC
  #include "ipc_utils.h"
  #include "utils.h"
#endif

static pid_t child;

bool ltr_int_fork_child(char *args[], bool *is_child)
{
  if((child = fork()) == 0){
    //Child here
    *is_child = true;
    execv(args[0], args);
    perror("execv");
    printf("Child should quit now...\n");
    return false;
  }
  return true;
}

#ifndef LIBLINUXTRACK_SRC
bool ltr_int_wait_child_exit(int limit)
{
  pid_t res;
  int status;
  int cntr = 0;
  do{
//    printf("Waiting!\n");
    ltr_int_usleep(100000);
    res = waitpid(child, &status, WNOHANG);
    ++cntr;
  }while((res != child) && (cntr < limit));
  if((res == child) && (WIFEXITED(status))){
    return true;
  }else{
    return false;
  }
}

//Check if some server is not running by trying to lock specific file.
// If psem is not NULL, return this way newly semaphore on the appropriate file.
// If should_lock is false, the file is not actually locked even if it can be.
//  Return values:
//    -1 error
//     0 server not running
//     1 server running
int ltr_int_server_running_already(const char *lockName, semaphore_p *psem, bool should_lock)
{
  char *lockFile;
  semaphore_p pfSem;
  int result = -1;
  lockFile = ltr_int_get_default_file_name(lockName);
  if(lockFile == NULL){
    ltr_int_log_message("Can't determine pref file path!\n");
    return -1;
  }
  pfSem = ltr_int_createSemaphore(lockFile);
  free(lockFile);
  if(pfSem == NULL){
    ltr_int_log_message("Can't create semaphore!");
    return -1;
  }
  bool lock_result;
  if(should_lock){
    lock_result = ltr_int_tryLockSemaphore(pfSem);
  }else{
    lock_result = ltr_int_testLockSemaphore(pfSem);
  }
  
  if(lock_result == false){
    ltr_int_log_message("Can't lock - server runs already!\n");
    result = 1;
  }else{
    result = 0;
  }
  if(psem != NULL){
    *psem = pfSem;
  }else{
    ltr_int_closeSemaphore(pfSem);
    pfSem= NULL;
  }
  return result;
}


#endif

struct semaphore_t{
  int fd;
}semaphore_t;

static semaphore_p ltr_int_semaphoreFromFd(int fd)
{
  semaphore_p res = (semaphore_p)ltr_int_my_malloc(sizeof(semaphore_t));
  res->fd = fd;
  return res;
}

#ifndef LIBLINUXTRACK_SRC
semaphore_p ltr_int_createSemaphore(char *fname)
{
  if(fname == NULL){
    return NULL;
  }
  int fd = open(fname, O_RDWR | O_CREAT, 0600);
  if(fd == -1){
    perror("open: ");
    return NULL;
  }
  return ltr_int_semaphoreFromFd(fd);
}
#endif

bool ltr_int_lockSemaphore(semaphore_p semaphore)
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


#ifndef LIBLINUXTRACK_SRC
bool ltr_int_tryLockSemaphore(semaphore_p semaphore)
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

//Differnce between last one and this is, that this one
// wouldn't actually lock the file!
bool ltr_int_testLockSemaphore(semaphore_p semaphore)
{
  if(semaphore == NULL){
    return false;
  }
  int res = lockf(semaphore->fd, F_TEST,0);
  if(res == 0){
    return true;
  }else{
    return false;
  }
}

#endif

bool ltr_int_unlockSemaphore(semaphore_p semaphore)
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

void ltr_int_closeSemaphore(semaphore_p semaphore)
{
  close(semaphore->fd);
  free(semaphore);
}

//the fname argument should end with XXXXXX;
//  it is also modified by the call to contain the actual filename.
//
//Returns opened file descriptor
int ltr_int_open_tmp_file(char *fname)
{
  umask(S_IWGRP | S_IWOTH);
  return mkstemp(fname);
}

//Closes and removes the file...
void ltr_int_close_tmp_file(char *fname, int fd)
{
  close(fd);
  unlink(fname);
}

char *ltr_int_get_com_file_name()
{
  return ltr_int_get_default_file_name("linuxtrack.comm");
}

static const char *mmapped_file_name()
{
  static const char name[] = "/tmp/ltr_mmapXXXXXX";
  return name;
}

static bool ltr_int_mmap(int fd, ssize_t tmp_size, struct mmap_s *m)
{
  //Check if file does have needed length...
  struct stat file_stat;
  bool truncate = true;
  if(fstat(fd, &file_stat) == 0){
    if(file_stat.st_size == (ssize_t)tmp_size){
      truncate = false;
    }
  }
  
  if(truncate){
    int res = ftruncate(fd, tmp_size);
    if (res == -1) {
      perror("ftruncate: ");
      close(fd);
      return false;
    }
  }
  m->data = mmap(NULL, tmp_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(m->data == (void*)-1){
    perror("mmap: ");
    close(fd);
    return false;
  }
  return true;
}

#ifndef LIBLINUXTRACK_SRC

bool ltr_int_mmap_file(const char *fname, size_t tmp_size, struct mmap_s *m)
{
  umask(S_IWGRP | S_IWOTH);
  int fd = open(fname, O_RDWR | O_CREAT | O_NOFOLLOW, 0700);
  if(fd < 0){
    perror("open: ");
    return false;
  }
  
  if(!ltr_int_mmap(fd, tmp_size, m)){
    close(fd);
    return false;
  }
  m->fname = ltr_int_my_strdup(fname);
  m->size = tmp_size;
  m->sem = ltr_int_semaphoreFromFd(fd);
  m->lock_sem = NULL;
  m->sem->fd = fd;
  return true;
}

#endif

bool ltr_int_mmap_file_exclusive(size_t tmp_size, struct mmap_s *m)
{
  umask(S_IWGRP | S_IWOTH);
  
  char *file_name = ltr_int_my_strdup(mmapped_file_name());
  int fd = ltr_int_open_tmp_file(file_name);
  if(fd < 0){
    perror("mkstemp");
    return false;
  }
  
  if(!ltr_int_mmap(fd, tmp_size, m)){
    ltr_int_close_tmp_file(file_name, fd);
    free(file_name);
    return false;
  }
  m->fname = file_name;
  m->size = tmp_size;
  m->sem = ltr_int_semaphoreFromFd(fd);
  m->lock_sem = NULL;
  m->sem->fd = fd;
  return true;
}

bool ltr_int_unmap_file(struct mmap_s *m)
{
  if(m->data == NULL){
    return true;
  }
  ltr_int_closeSemaphore(m->sem);
  if(m->lock_sem != NULL){
    ltr_int_closeSemaphore(m->lock_sem);
  }
  int res = munmap(m->data, m->size);
  m->data = NULL;
  m->size = 0;
  if(res < 0){
    perror("munmap: ");
  }
  unlink(m->fname);
  if(m->fname != NULL){
    free(m->fname);
    m->fname = NULL;
  }
  return res == 0;
}



bool ltr_int_make_fifo(const char *name)
{
  if(name == NULL){
    printf("Name must be set! (NULL passed)\n");
  }
  struct stat info;
  if(stat(name, &info) == 0){
    //file exists, check if it is fifo...
    if(S_ISFIFO(info.st_mode)){
      printf("Fifo exists already!\n");
      return true;
    }else if(S_ISDIR(info.st_mode)){
      printf("Directory exists! Will try to remove it.\n");
      if(rmdir(name) != 0){
        perror("rmdir");
        return false;
      }
    }else{
      printf("File exists, but it is not a fifo! Will try to remove it.\n");
      if(unlink(name) != 0){
        perror("unlink");
        return false;
      }
    }
  }
  //At this point, the file should not exist.
  if(mkfifo(name, S_IRUSR | S_IWUSR) != 0){
    perror("mkfifo");
    return false;
  }
  printf("Fifo created!\n");
  return true;
}

int ltr_int_open_fifo_exclusive(const char *name)
{
  if(!ltr_int_make_fifo(name)){
    return -1;
  }
  int fifo = -1;
  if((fifo = open(name, O_RDONLY | O_NONBLOCK)) > 0){
    ltr_int_log_message("Fifo %s opened!\n", name);
    if(flock(fifo, LOCK_EX | LOCK_NB) == 0){
      ltr_int_log_message("Fifo %s (fd %d) opened and locked!\n", name, fifo);
      return fifo;
    }
    ltr_int_log_message("Fifo %s (fd %d) could not be locked, closing it!\n", name, fifo);
    close(fifo);
  }
  return -1;
}

int ltr_int_open_fifo_for_writing(const char *name, bool wait){
  if(!ltr_int_make_fifo(name)){
    return -1;
  }
  
  //Open the pipe (handling the wait for the other party to open reading end)
  //  Todo: add some timeout?
  int fifo = -1;
  int timeout = 3;
  while(timeout > 0){
    --timeout;
    if((fifo = open(name, O_WRONLY | O_NONBLOCK)) < 0){
      if(errno != ENXIO){
        perror("open@open_fifo_for_writing");
        return -1;
      }
      fifo = -1;
      if(!wait){
        return -1;
      }
      sleep(1);
    }else{
      break;
    }
  }
  return fifo;
}

int ltr_int_open_unique_fifo(char **name, int *num, const char *template, int max)
{
  //although I could come up with method allowing more than 100
  //  fifos to be checked, there is not a point doing so
  int i;
  char *fifo_name = NULL;
  int fifo = -1;
  for(i = 0; i < max; ++i){
    asprintf(&fifo_name, template, i);
    fifo = ltr_int_open_fifo_exclusive(fifo_name);
    if(fifo > 0){
      break;
    }
    free(fifo_name);
    fifo_name = NULL;
  }
  *name = fifo_name;
  *num = i;
  return fifo;
}

int ltr_int_fifo_send(int fifo, void *buf, size_t size)
{
  if(fifo <= 0){
    return -1;
  }
  if(write(fifo, buf, size) < 0){
    printf("Write @fd %d failed:\n", fifo);
    perror("write@pipe_send");
    return -errno;
   }
  return 0;
}

ssize_t ltr_int_fifo_receive(int fifo, void *buf, size_t size)
{
  //Assumption is, that write/read of less than 512 bytes should be atomic...
  ssize_t num_read = read(fifo, buf, size);
  if(num_read > 0){
    return num_read;
  }else if(num_read < 0){
    perror("read@fifo_receive");
    return -errno;
  }
  return 0;
}


// Return value
//   0 - timed out
//   1 - data available
//  -1 - problem (HUP or so)
int ltr_int_pipe_poll(int pipe, int timeout, bool *hup)
{
  struct pollfd downlink_poll= {
    .fd = pipe,
    .events = POLLIN,
    .revents = 0
  };
  if(hup != NULL){
    *hup = false;
  }
  int fds = poll(&downlink_poll, 1, timeout);
  if(fds > 0){
    if(downlink_poll.revents & POLLHUP){
      if(hup != NULL){
        *hup = true;
      }
      return -1;
    }
    return fds;
  }else if(fds < 0){
    perror("poll");
    return -1;
  }
  return 0;
}
