
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

#include "ipc_utils.h"
#include "utils.h"

static pid_t child;

bool ltr_int_fork_child(char *args[], bool *is_child)
{
  if((child = fork()) == 0){
    //Child here
    *is_child = true;
    execv(args[0], args);
    ltr_int_my_perror("execv");
    ltr_int_log_message("Child should quit now...\n");
    return false;
  }
  return true;
}

bool ltr_int_wait_child_exit(int limit)
{
  pid_t res;
  int status;
  int cntr = 0;
  do{
//    ltr_int_log_message("Waiting!\n");
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

bool ltr_int_child_alive()
{
  pid_t res;
  int status;
  res = waitpid(child, &status, WNOHANG);
  return (res != child);
}

//Check if some server is not running by trying to lock specific file.
// If psem is not NULL, return this way newly semaphore on the appropriate file.
// If should_lock is false, the file is not actually locked even if it can be.
//  Return values:
//    -1 error
//     0 server not running
//     1 server running
int ltr_int_server_running_already(const char *lockName, bool isAbsolute, 
                                   semaphore_p *psem, bool should_lock)
{
  char *lockFile;
  semaphore_p pfSem;
  int result = -1;
  if(isAbsolute){
    lockFile = ltr_int_my_strdup(lockName);
  }else{
    lockFile = ltr_int_get_default_file_name(lockName);
  }
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
    ltr_int_log_message("Passing the lock to protect fifo (pid %d)!\n", getpid());
    *psem = pfSem;
  }else{
    ltr_int_closeSemaphore(pfSem);
    pfSem= NULL;
  }
  return result;
}



struct semaphore_t{
  int fd;
}semaphore_t;

static semaphore_p ltr_int_semaphoreFromFd(int fd)
{
  semaphore_p res = (semaphore_p)ltr_int_my_malloc(sizeof(semaphore_t));
  res->fd = fd;
  return res;
}

semaphore_p ltr_int_createSemaphore(char *fname)
{
  if(fname == NULL){
    return NULL;
  }
  int fd = open(fname, O_RDWR | O_CREAT, 0600);
  if(fd == -1){
    ltr_int_my_perror("open: ");
    return NULL;
  }
  ltr_int_log_message("Going to create lock '%s' => %d!\n", fname, fd);
  return ltr_int_semaphoreFromFd(fd);
}

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


bool ltr_int_tryLockSemaphore(semaphore_p semaphore)
{
  if(semaphore == NULL){
    return false;
  }
  int res = lockf(semaphore->fd, F_TLOCK,0);
  if(res == 0){
    ltr_int_log_message("Lock %d success!\n", semaphore->fd);
    return true;
  }else{
    ltr_int_log_message("Can't lock %d!\n", semaphore->fd);
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
  ltr_int_log_message("Closing semaphore %d (pid %d)!\n", semaphore->fd, getpid());
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

/*
char *ltr_int_get_com_file_name()
{
  return ltr_int_get_default_file_name("linuxtrack.comm");
}
*/

static const char *mmapped_file_name()
{
  static const char name[] = "/tmp/ltr_mmapXXXXXX";
  return name;
}

static bool ltr_int_mmap(int fd, ssize_t tmp_size, struct mmap_s *m)
{
  //Check if file does have needed length...
  struct stat file_stat;
  bool doTruncate = true;
  if(fstat(fd, &file_stat) == 0){
    if(file_stat.st_size == (ssize_t)tmp_size){
      doTruncate = false;
    }
  }
  
  if(doTruncate){
    int res = ftruncate(fd, tmp_size);
    if (res == -1) {
      ltr_int_my_perror("ftruncate: ");
      close(fd);
      return false;
    }
  }
  m->data = mmap(NULL, tmp_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(m->data == (void*)-1){
    ltr_int_my_perror("mmap: ");
    close(fd);
    return false;
  }
  return true;
}

bool ltr_int_mmap_file(const char *fname, size_t tmp_size, struct mmap_s *m)
{
  umask(S_IWGRP | S_IWOTH);
  int fd = open(fname, O_RDWR | O_CREAT | O_NOFOLLOW, 0700);
  if(fd < 0){
    ltr_int_my_perror("open: ");
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

bool ltr_int_mmap_file_exclusive(size_t tmp_size, struct mmap_s *m)
{
  umask(S_IWGRP | S_IWOTH);
  
  char *file_name = ltr_int_my_strdup(mmapped_file_name());
  int fd = ltr_int_open_tmp_file(file_name);
  if(fd < 0){
    ltr_int_my_perror("mkstemp");
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
    ltr_int_my_perror("munmap: ");
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
    ltr_int_log_message("Name must be set! (NULL passed)\n");
  }
  struct stat info;
  if(stat(name, &info) == 0){
    //file exists, check if it is fifo...
    if(S_ISFIFO(info.st_mode)){
      ltr_int_log_message("Fifo exists already!\n");
      return true;
    }else if(S_ISDIR(info.st_mode)){
      ltr_int_log_message("Directory exists! Will try to remove it.\n");
      if(rmdir(name) != 0){
        ltr_int_my_perror("rmdir");
        return false;
      }
    }else{
      ltr_int_log_message("File exists, but it is not a fifo! Will try to remove it.\n");
      if(unlink(name) != 0){
        ltr_int_my_perror("unlink");
        return false;
      }
    }
  }
  //At this point, the file should not exist.
  if(mkfifo(name, S_IRUSR | S_IWUSR) != 0){
    ltr_int_my_perror("mkfifo");
    return false;
  }
  ltr_int_log_message("Fifo created!\n");
  return true;
}

int ltr_int_open_fifo_exclusive(const char *name, semaphore_p *lock_sem)
{
  char *lock_name = NULL;
  if(asprintf(&lock_name, "%s.lock", name) == -1){
    return -1;
  }
  if(ltr_int_server_running_already(lock_name, true, lock_sem, true) != 0){
    ltr_int_log_message("Fifo %s (lock %s) could not be locked, closing it!\n", name, lock_name);
    return -1;
  }
  ltr_int_log_message("Fifo %s (lock %s) locked!!!", name, lock_name);
  free(lock_name);
  if(!ltr_int_make_fifo(name)){
    ltr_int_unlockSemaphore(*lock_sem);
    ltr_int_closeSemaphore(*lock_sem);
    ltr_int_log_message("Fifo %s could not be created!\n", name);
    return -1;
  }
  int fifo = -1;
  if((fifo = open(name, O_RDONLY | O_NONBLOCK)) > 0){
    ltr_int_log_message("Fifo %s (fd %d) opened and locked!\n", name, fifo);
    return fifo;
  }
  return -1;
}

int ltr_int_open_fifo_for_writing(const char *name, bool waitForOpen){
  //ltr_int_log_message("Trying to open fifo '%s'...\n", name);
  if(!ltr_int_make_fifo(name)){
    ltr_int_log_message("Failed to create fifo for writing!\n");
    return -1;
  }
  
  //Open the pipe (handling the wait for the other party to open reading end)
  //  Todo: add some timeout?
  int fifo = -1;
  int timeout = 3;
  while(timeout > 0){
    --timeout;
    if((fifo = open(name, O_WRONLY | O_NONBLOCK)) < 0){
      ltr_int_my_perror("open_fifo_for_writing");
      //ltr_int_log_message("Fifo for writing failed to open (%s)!\n", name);
      if(errno != ENXIO){
        ltr_int_my_perror("open@open_fifo_for_writing");
        return -1;
      }
      fifo = -1;
      if(!waitForOpen){
        return -1;
      }
      sleep(1);
    }else{
      break;
    }
  }
  return fifo;
}

int ltr_int_open_unique_fifo(char **name, int *num, const char *template, int max, semaphore_p *lock_sem)
{
  //although I could come up with method allowing more than 100
  //  fifos to be checked, there is not a point doing so
  int i;
  char *fifo_name = NULL;
  int fifo = -1;
  for(i = 0; i < max; ++i){
    if(asprintf(&fifo_name, template, i) > 0){
      fifo = ltr_int_open_fifo_exclusive(fifo_name, lock_sem);
      if(fifo > 0){
        break;
      }
      free(fifo_name);
      fifo_name = NULL;
    }
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
    ltr_int_log_message("Write @fd %d failed:\n", fifo);
    int err = errno;
    ltr_int_my_perror("write@pipe_send");
    return -err;
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
    ltr_int_my_perror("read@fifo_receive");
    return -errno;
  }
  return 0;
}


// Return value
//   0 - timed out
//   1 - data available
//  -1 - problem (HUP or so)
int ltr_int_pipe_poll(int pipefd, int timeout, bool *hup)
{
  struct pollfd downlink_poll= {
    .fd = pipefd,
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
    ltr_int_my_perror("poll");
    return -1;
  }
  return 0;
}


