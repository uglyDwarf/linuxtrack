#ifndef IPC_UTILS__H
#define IPC_UTILS__H

#include <stdbool.h>

#ifndef LIBLINUXTRACK_SRC
  #define LIBLINUXTRACK_PRIVATE
#else
  #define LIBLINUXTRACK_PRIVATE static
#endif

LIBLINUXTRACK_PRIVATE bool fork_child(char *args[]);
bool wait_child_exit(int limit);

typedef struct semaphore_t *semaphore_p;
struct mmap_s{
  char *fname;
  size_t size;
  void *data;
  semaphore_p sem;
};

semaphore_p createSemaphore(char *fname);
LIBLINUXTRACK_PRIVATE bool lockSemaphore(semaphore_p semaphore);
bool tryLockSemaphore(semaphore_p semaphore);
LIBLINUXTRACK_PRIVATE bool unlockSemaphore(semaphore_p semaphore);
void closeSemaphore(semaphore_p semaphore);

LIBLINUXTRACK_PRIVATE bool mmap_file(const char *fname, size_t tmp_size, struct mmap_s *m);
bool unmap_file(struct mmap_s *m);
LIBLINUXTRACK_PRIVATE int open_tmp_file(char *fname);
void close_tmp_file(char *fname, int fd);


#endif
