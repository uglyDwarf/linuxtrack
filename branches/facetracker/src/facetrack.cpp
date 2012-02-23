#include <vector>
#include <iostream>
#include "facetrack.h"

#include <opencv/cv.h>

#include <pthread.h>
#include <string.h>
#include <stdio.h>

#include <utils.h>

static cv::CascadeClassifier *cascade = NULL;
static double scale = 1.0;

static float face_x = 0;
static float face_y = 0;

static int frame_size = 0;
static int frame_w = 0;
static int frame_h = 0;
static uint8_t *frame = NULL;
static cv::Mat *cvimage;


double expfilt(double x, 
              double y_minus_1,
              double filterfactor);

void detect(cv::Mat& img)
{
  static float last_face_x = 0.0f;
  static float last_face_y = 0.0f;
  std::vector<cv::Rect> faces;
  std::cout<<"Detection start..."<<std::endl;
  cv::equalizeHist(img, img);
  cascade->detectMultiScale(img, faces, 1.2, 2, 0, cv::Size(40, 40));
  double area = -1;
  const cv::Rect *candidate = NULL;
//  std::cout<<"Have "<<faces.size()<<"Candidates"<<std::endl;
  for(std::vector<cv::Rect>::const_iterator i = faces.begin(); i != faces.end(); ++i){
    if(i->width * i->height > area){
      candidate = &(*i);
      area = i->width * i->height;
    }
  }
  if(candidate != NULL){
    float x = (candidate->x + candidate->width / 2) - frame_w/2;
    float y = (candidate->y + candidate->height / 2) - frame_h/2;
    face_x = expfilt(x, last_face_x, 0.2);
    face_y = expfilt(y, last_face_y, 0.2);
    last_face_x = face_x;
    last_face_y = face_y;
  }
//  std::cout<<"Done" <<std::endl;
}

static bool run = true;
static enum {READY, PROCESSING, DONE} frame_status  = DONE;
static bool request_frame = false;
static pthread_cond_t frame_cv = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t frame_mx = PTHREAD_MUTEX_INITIALIZER;
static int pok1=12345;
static pthread_t detect_thread_handle;
static int pok2=54321;


void *detector_thread(void *)
{
  cascade = new cv::CascadeClassifier();
  cascade->load("/usr/share/opencv/haarcascades/haarcascade_frontalface_alt2.xml");
  while(run){
    pthread_mutex_lock(&frame_mx);
    while(frame_status != READY){
      pthread_cond_wait(&frame_cv, &frame_mx);
    }
    if(!run){
      break;
    }
    frame_status = PROCESSING;
    pthread_mutex_unlock(&frame_mx);
    detect(*cvimage);
    pthread_mutex_lock(&frame_mx);
    frame_status = DONE;
    pthread_mutex_unlock(&frame_mx);
  }
  delete cascade;
  delete cvimage;
  free(frame);
}


void stop_detect()
{
  run = false;
  pthread_mutex_lock(&frame_mx);
  frame_status = READY;
  pthread_cond_broadcast(&frame_cv);
  pthread_mutex_unlock(&frame_mx);
  pthread_join(detect_thread_handle, NULL);
  ltr_int_log_message("Facetracker thread joined!\n");
  fflush(stderr);
}

void face_detect(image *img, struct bloblist_type *blt)
{
  static bool init = true;
  if(init){
    cv::setNumThreads(1);
    pthread_create(&detect_thread_handle, NULL, detector_thread, NULL);
    init = false;
  }
  
  if((frame_w != img->w) || (frame_h != img->h) || (frame == NULL)){
    if(frame != NULL){
      free(frame);
    }
    if(cvimage != NULL){
      delete cvimage;
    }
    frame_w = img->w;
    frame_h = img->h;
    frame = (uint8_t*)malloc(frame_w * frame_h);
    cvimage = new cv::Mat(frame_h, frame_w, CV_8U, frame);
  }
  if(frame_status == DONE){
    memcpy(frame, img->bitmap, frame_w * frame_h);
    pthread_mutex_lock(&frame_mx);
    frame_status = READY;
    pthread_cond_broadcast(&frame_cv);
    pthread_mutex_unlock(&frame_mx);
  }
  blt->num_blobs = 1;
  blt->blobs[0].x = face_x;
  blt->blobs[0].y = face_y;
  ltr_int_draw_cross(img, face_x + frame_w/2, face_y + frame_h/2, 20);
}


double expfilt(double x, 
              double y_minus_1,
              double filterfactor) 
{
  double y;
  
  y = y_minus_1*(1.0-filterfactor) + filterfactor*x;
 return y;
}

