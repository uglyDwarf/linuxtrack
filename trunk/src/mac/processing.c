#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>

#include "image_process.h"

#ifdef OPENCV
#include "facetrack.h"
#endif

#include <cal.h>
#include <ipc_utils.h>
#include "buffer.h"
#include <com_proc.h>
#include <args.h>

static pthread_cond_t state_cv = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t state_mx = PTHREAD_MUTEX_INITIALIZER;
static bool new_frame_flag = false;
static pthread_t processing_thread;
static bool end_flag = false;

static struct mmap_s *mmm;

static int width = -1;
static int height = -1;
static int reader = 0;
static int writer = 0;

#ifdef OPENCV
static bool facetrack = false;
#endif

static void *processingThreadFun(void *param)
{
  (void) param;
  
  while(!end_flag){
    pthread_mutex_lock(&state_mx);
    while((!new_frame_flag) && (!end_flag)){
		struct timeval now;
		gettimeofday(&now, NULL);
		struct timespec absTime;
		absTime.tv_sec = now.tv_sec + 3;  
		absTime.tv_nsec = now.tv_usec * 1000;
      pthread_cond_timedwait(&state_cv, &state_mx, &absTime);
    }
    new_frame_flag = false;
    pthread_mutex_unlock(&state_mx);
//    printf("Processing frame!\n");
    if(isEmpty(reader)){
//      printf("No new buffer!\n");
    }else{
//      printf("Processing buffer %d @ %p\n", reader, getCurrentBuffer(reader));
      image img = {
	    .bitmap = getCurrentBuffer(reader),
	    .w = width,
	    .h = height,
	    .ratio = 1.0f
      };
      struct blob_type blobs_array[3] = {
        {0.0f, 0.0f, -1},
        {0.0f, 0.0f, -1},
        {0.0f, 0.0f, -1}
      };
      struct bloblist_type bloblist = {
       .num_blobs = 3,
       .blobs = blobs_array
      };
      
      bool res;
#ifdef OPENCV
      if(facetrack){ 
        ltr_int_face_detect(&img, &bloblist);
        res = (bloblist.num_blobs == 1);
      }else{
#endif
        ltr_int_to_stripes(&img);
        res = (ltr_int_stripes_to_blobs(3, &bloblist, ltr_int_getMinBlob(mmm), ltr_int_getMaxBlob(mmm), &img) == 0);
#ifdef OPENCV
      }
#endif
      if(res){
	    ltr_int_setBlobs(mmm, blobs_array, bloblist.num_blobs);
        if(!ltr_int_getFrameFlag(mmm)){
//	  printf("Copying buffer!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	      memcpy(ltr_int_getFramePtr(mmm), img.bitmap, width * height);
	      ltr_int_setFrameFlag(mmm);
	    }
      }
      bufferRead(&reader);
    }
  }
#ifdef OPENCV
  if(facetrack){
    ltr_int_stop_face_detect();
  }
  ltr_int_cleanup_after_processing();
#endif
  return NULL;
}

bool startProcessing(int w, int h, int buffers, struct mmap_s *mmm_p)
{
  mmm = mmm_p;
  new_frame_flag = 0;
  end_flag = false;
  width = w;
  height = h;
  if(!createBuffers(buffers, w * h)){
//    printf("Problem creating buffers!\n");
    return false;
  }
  ltr_int_prepare_for_processing(w, h);
#ifdef OPENCV
  facetrack = doFacetrack();
  if(facetrack && (!ltr_int_init_face_detect())){
    return false;
  }
#endif
//  printf("Starting processing thread!\n");
  return 0 == pthread_create(&processing_thread, NULL, processingThreadFun, NULL);
}

void endProcessing()
{
//  printf("Signaling end to the processing thread!\n");
  end_flag = true;
  pthread_join(processing_thread, NULL);
}

bool newFrame(unsigned char *ptr)
{
  if(!isEmpty(writer)){
//    printf("No empty buffer!\n");
    return false;
  }
  
  unsigned char *dest = getCurrentBuffer(writer);
//  printf("Writing buffer %d @ %p\n", writer, dest);
  unsigned char thr;
  
#ifdef OPENCV
  if(facetrack){
    thr = (unsigned char)0;
  }else{
#endif
    thr = (unsigned char)ltr_int_getThreshold(mmm);
#ifdef OPENCV
  }
#endif
  size_t i;
  for(i = 0; i < (size_t) width * height; ++i){
    dest[i] = (*ptr >= thr) ? *ptr : 0;
    ptr += 2;
  }
  bufferWritten(&writer);
  
//  printf("Signaling new frame!\n");
  pthread_mutex_lock(&state_mx);
  new_frame_flag = true;
  pthread_cond_broadcast(&state_cv);
  pthread_mutex_unlock(&state_mx);
  return true;
}

const char *ltr_int_wc_get_cascade()
{
  return getCascadeFileName();
}

int ltr_int_wc_get_optim_level()
{
  return ltr_int_getOptLevel(mmm);
}

float ltr_int_wc_get_eff()
{
  return ltr_int_getEff(mmm);
}

