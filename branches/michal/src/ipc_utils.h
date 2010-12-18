#ifndef IPC_UTILS__H
#define IPC_UTILS__H

#include <stdbool.h>

bool fork_child(char *args[]);
bool wait_child_exit(int limit);

typedef struct semaphore_t *semaphore_p;
struct mmap_s{
  char *fname;
  size_t size;
  void *data;
  semaphore_p sem;
};

semaphore_p createSemaphore(char *fname);
bool lockSemaphore(semaphore_p semaphore);
bool tryLockSemaphore(semaphore_p semaphore);
bool unlockSemaphore(semaphore_p semaphore);
void closeSemaphore(semaphore_p semaphore);

bool mmap_file(const char *fname, size_t tmp_size, struct mmap_s *m);
bool unmap_file(struct mmap_s *m);
int open_tmp_file(char *fname);
int open_tmp_file(char *fname);


#endif