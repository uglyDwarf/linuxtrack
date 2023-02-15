
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
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "ipc_utils.h"
#include "utils.h"

//UNIX_PATH_MAX is defined in linux/un.h, but that doesn't seem portable.
//  So I use this instead...
struct sockaddr_un sizecheck;
#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX sizeof(sizecheck.sun_path)
#endif


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



int ltr_int_make_socket(const char *name)
{
  if(name == NULL){
    ltr_int_log_message("Name must be set! (NULL passed)\n");
    return -1;
  }
  if(strlen(name) > UNIX_PATH_MAX - 2){
    ltr_int_log_message("Socket name '%s' too long (max. %d)\n", name, UNIX_PATH_MAX - 2);
    return -1;
  }
  struct stat info;
  if(stat(name, &info) == 0){
    //file exists, check if it is socket...
    if(S_ISSOCK(info.st_mode)){
      ltr_int_log_message("Socket exists already!\n");
      int tmp_sock = ltr_int_connect_to_socket(name);
      if(tmp_sock < 0){
        ltr_int_log_message("Couldn't connect, will try removing the socket!\n");
        if(unlink(name) != 0){
          ltr_int_my_perror("unlink");
          return -1;
        }
      }else{
        ltr_int_log_message("Master is responding on the other end!\n");
        shutdown(tmp_sock, SHUT_RDWR);
        close(tmp_sock);
        return -1;
      }
    }else if(S_ISDIR(info.st_mode)){
      ltr_int_log_message("Directory exists! Will try to remove it.\n");
      if(rmdir(name) != 0){
        ltr_int_my_perror("rmdir");
        return -1;
      }
    }else{
      ltr_int_log_message("File exists, but it is not a socket! Will try to remove it.\n");
      if(unlink(name) != 0){
        ltr_int_my_perror("unlink");
        return -1;
      }
    }
  }
  struct sockaddr_un address;
  int socket_fd = -1;

  socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if(socket_fd < 0){
    ltr_int_my_perror("socket");
    return -1;
  }
  int set = 1;
  if(ioctl(socket_fd, FIONBIO, (char *)&set) < 0){
    perror("ioctl");
    close(socket_fd);
    return -1;
  }
  memset(&address, 0, sizeof(struct sockaddr_un));
  address.sun_family = AF_UNIX;
  strncpy(address.sun_path, name, UNIX_PATH_MAX - 1);

  //At this point, the file should not exist.
  if(bind(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0){
    ltr_int_my_perror("bind");
    return -1;
  }
  if(listen(socket_fd, 1) != 0){
    ltr_int_my_perror("listen");
    return -1;
  }
  ltr_int_log_message("Socket created!\n");
  return socket_fd;
}



int ltr_int_create_server_socket(const char *name){
  //ltr_int_log_message("Trying to open fifo '%s'...\n", name);
  int fifo = -1;
  fifo = ltr_int_make_socket(name);
  if(fifo < 0){
    ltr_int_log_message("Failed to create socket!\n");
    return -1;
  }
  return fifo;
}


int ltr_int_connect_to_socket(const char *name){
  if(strlen(name) > UNIX_PATH_MAX - 2){
    ltr_int_log_message("Socket name '%s' too long (max. %d)\n", name, UNIX_PATH_MAX - 2);
    return -1;
  }
  //ltr_int_log_message("Trying to open fifo '%s'...\n", name);
  printf("Will try to connect to socket '%s'\n", name);
  struct sockaddr_un address;
  int socket_fd = -1;

  socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if(socket_fd < 0){
    ltr_int_my_perror("socket");
    return -1;
  }
  int set = 1;
  if(ioctl(socket_fd, FIONBIO, (char *)&set) < 0){
    perror("ioctl");
    close(socket_fd);
    return -1;
  }
  memset(&address, 0, sizeof(struct sockaddr_un));
  address.sun_family = AF_UNIX;
  strncpy(address.sun_path, name, UNIX_PATH_MAX - 1);

  if(connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0){
    ltr_int_my_perror("connect");
    return -1;
  }

  if(socket_fd < 0){
    ltr_int_log_message("Failed to connect to socket!\n");
    return -1;
  }
  return socket_fd;
}


int ltr_int_socket_send(int socket, void *buf, size_t size)
{
  if(socket <= 0){
    return -1;
  }
  if(write(socket, buf, size) < 0){
    ltr_int_log_message("Write @fd %d failed:\n", socket);
    int err = errno;
    ltr_int_my_perror("write@socket_send");
    return -err;
   }
  return 0;
}

ssize_t ltr_int_socket_receive(int socket, void *buf, size_t size)
{
  //Assumption is, that write/read of less than 512 bytes should be atomic...
  ssize_t num_read = read(socket, buf, size);
  if(num_read > 0){
    return num_read;
  }else if(num_read < 0){
    ltr_int_my_perror("read@socket_receive");
    return -errno;
  }
  return 0;
}

int ltr_int_close_socket(int socket)
{
  shutdown(socket, SHUT_RDWR);
  close(socket);
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


