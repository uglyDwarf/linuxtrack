#ifndef COM_PROC__H
#define COM_PROC__H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <ipc_utils.h>

typedef enum {STOP, SLEEP, WAKEUP} command_t;

typedef struct{
  float x, y;
  int score;
} blobs_t;

struct blob_type;

command_t ltr_int_getCommand(struct mmap_s *mmm);
void ltr_int_setCommand(struct mmap_s *mmm, command_t cmd);
int ltr_int_getThreshold(struct mmap_s *mmm);
void ltr_int_setThreshold(struct mmap_s *mmm, int thr);
int ltr_int_getMinBlob(struct mmap_s *mmm);
void ltr_int_setMinBlob(struct mmap_s *mmm, int pix);
int ltr_int_getMaxBlob(struct mmap_s *mmm);
void ltr_int_setMaxBlob(struct mmap_s *mmm, int pix);
void ltr_int_setBlobs(struct mmap_s *mmm, struct blob_type *b, int num_blobs);
bool ltr_int_haveNewBlobs(struct mmap_s *mmm);
int ltr_int_getBlobs(struct mmap_s *mmm, struct blob_type * b);
unsigned char* ltr_int_getFramePtr(struct mmap_s *mmm);
bool ltr_int_getFrameFlag(struct mmap_s *mmm);
void ltr_int_setFrameFlag(struct mmap_s *mmm);
void ltr_int_resetFrameFlag(struct mmap_s *mmm);
void ltr_int_printCmd(char *prefix, command_t cmd);
void ltr_int_setWiiIndication(struct mmap_s *mmm, int new_ind);
int ltr_int_getWiiIndication(struct mmap_s *mmm);
int ltr_int_get_com_size();

#ifdef __cplusplus
}
#endif

#endif
