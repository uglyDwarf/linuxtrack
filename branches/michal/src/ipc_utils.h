#ifndef IPC_UTILS__H
#define IPC_UTILS__H

#include <stdbool.h>

#ifndef LIBLINUXTRACK_SRC
  #define LIBLINUXTRACK_PRIVATE
#else
  #define LIBLINUXTRACK_PRIVATE static
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct semaphore_t *semaphore_p;
struct mmap_s{
  char *fname;
  size_t size;
  void *data;
  semaphore_p sem;
};

semaphore_p ltr_int_server_running_already(char *lockName);
LIBLINUXTRACK_PRIVATE bool ltr_int_fork_child(char *args[]);
bool ltr_int_wait_child_exit(int limit);

semaphore_p ltr_int_createSemaphore(char *fname);
LIBLINUXTRACK_PRIVATE bool ltr_int_lockSemaphore(semaphore_p semaphore);
bool ltr_int_tryLockSemaphore(semaphore_p semaphore);
LIBLINUXTRACK_PRIVATE bool ltr_int_unlockSemaphore(semaphore_p semaphore);
void ltr_int_closeSemaphore(semaphore_p semaphore);

LIBLINUXTRACK_PRIVATE bool ltr_int_mmap_file(const char *fname, size_t tmp_size, struct mmap_s *m);
bool ltr_int_unmap_file(struct mmap_s *m);
int ltr_int_open_tmp_file(char *fname);
void ltr_int_close_tmp_file(char *fname, int fd);
LIBLINUXTRACK_PRIVATE char *ltr_int_get_com_file_name();

#ifdef __cplusplus
}
#endif

#endif
