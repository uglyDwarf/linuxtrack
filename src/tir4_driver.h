#ifndef TIR4_DRIVER__H
#define TIR4_DRIVER__H

#include <stdbool.h>
#include <usb.h>

/********************/
/* public constants */
/********************/
#define TIR4_VERTICAL_SQUASHED_RESOLUTION 288

/* image width and height */ 
#define TIR4_HORIZONTAL_RESOLUTION 710
#define TIR4_VERTICAL_RESOLUTION (TIR4_VERTICAL_SQUASHED_RESOLUTION * 2)

/*********************/
/* public data types */
/*********************/
struct tir4_blob_type {
  /* coordinates of the blob on the screen */
  float x,y; 
  /* total # pixels area, used for sorting/scoring blobs */
  unsigned int area; 
};

struct tir4_bloblist_type {
  struct tir4_bloblist_iter *head;
  struct tir4_bloblist_iter *tail;
};

struct tir4_bloblist_iter {
  struct tir4_blob_type blob;
  struct tir4_bloblist_iter *next;
  struct tir4_bloblist_iter *prev;
};

struct tir4_frame_type {
  struct tir4_bloblist_type  bloblist; 
  unsigned int num_blobs;
/*   char **bitmap; /\* FIXME: add bitmap support *\/ */
};

/******************************/
/* public function prototypes */
/******************************/
/* call to init an uninitialized tir4 device 
 * typically called once at setup
 * turns the IR leds on
 * this function may block for up to 1 second 
 */
void tir4_init(void);

/* call to shutdown the tir4 device 
 * typically called once at close
 * turns the IR leds off
 * can be used to deactivate the tir4;
 * must call init to restart */
void tir4_shutdown(void);

/* this controls the tir4 red and green LED
 * typically called whenever the tracking is "good"
 * when called with true, the green led is lit
 * when called with false, the red led is lit
 * neither is lit immediatly after init!
 */
void tir4_set_good_indication(bool arg);

void tir4_do_read(void);

bool tir4_frame_is_available(void);

void tir4_get_frame(struct tir4_frame_type *f);

void tir4_frame_print(struct tir4_frame_type *f);
void tir4_blob_print(struct tir4_blob_type b);
void tir4_bloblist_init(struct tir4_bloblist_type *t4bl);
void tir4_bloblist_delete(struct tir4_bloblist_type *t4bl,
                          struct tir4_bloblist_iter *t4bli);
struct tir4_bloblist_iter *tir4_bloblist_get_iter(struct tir4_bloblist_type *t4bl);
struct tir4_bloblist_iter *tir4_bloblist_iter_next(struct tir4_bloblist_iter *t4bli);
bool tir4_bloblist_iter_complete(struct tir4_bloblist_iter *t4bli);
void tir4_bloblist_free(struct tir4_bloblist_type *t4bl);
void tir4_bloblist_add_tir4_blob(struct tir4_bloblist_type *t4bl,
                                 struct tir4_blob_type b);
void tir4_bloblist_print(struct tir4_bloblist_type *t4bl);

#endif
