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
  semaphore_p lock_sem;
};

int ltr_int_server_running_already(const char *lockName, semaphore_p *psem, bool should_lock);
LIBLINUXTRACK_PRIVATE bool ltr_int_fork_child(char *args[]);
bool ltr_int_wait_child_exit(int limit);

semaphore_p ltr_int_createSemaphore(char *fname);
LIBLINUXTRACK_PRIVATE bool ltr_int_lockSemaphore(semaphore_p semaphore);
bool ltr_int_tryLockSemaphore(semaphore_p semaphore);
bool ltr_int_testLockSemaphore(semaphore_p semaphore);
LIBLINUXTRACK_PRIVATE bool ltr_int_unlockSemaphore(semaphore_p semaphore);
LIBLINUXTRACK_PRIVATE void ltr_int_closeSemaphore(semaphore_p semaphore);

LIBLINUXTRACK_PRIVATE bool ltr_int_mmap_file(const char *fname, size_t tmp_size, struct mmap_s *m);
LIBLINUXTRACK_PRIVATE bool ltr_int_unmap_file(struct mmap_s *m);
int ltr_int_open_tmp_file(char *fname);
void ltr_int_close_tmp_file(char *fname, int fd);
LIBLINUXTRACK_PRIVATE char *ltr_int_get_com_file_name();

bool ltr_int_make_fifo(const char *name);
int ltr_int_open_fifo_exclusive(const char *name);
int ltr_int_open_fifo_for_writing(const char *name, bool wait);
int ltr_int_open_unique_fifo(char **name, int *num, const char *name_template, int max);
int ltr_int_fifo_send(int fifo, void *buf, size_t size);
ssize_t ltr_int_fifo_receive(int fifo, void *buf, size_t size);
int ltr_int_pipe_poll(int pipe, int timeout, bool *hup);

#ifdef __cplusplus
}
#endif

#endif
