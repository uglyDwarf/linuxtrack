#include <vector>
#include <iostream>
#include "facetrack.h"
#include "wc_driver_prefs.h"

#include <cv.h>

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <utils.h>

static cv::CascadeClassifier *cascade = NULL;
static double scale = 0.5;

static float face_x = 0;
static float face_y = 0;
static float face_w = 0;
static float face_h = 0;

static int frame_size = 0;
static int frame_w = 0;
static int frame_h = 0;
static uint8_t *frame = NULL;
static cv::Mat *cvimage;
static cv::Mat scaled;
static cv::Size minFace(40, 40);
float expFiltFactor = 0.2;
bool init = true;


float expfilt(float x, 
              float y_minus_1,
              float filterfactor);

void detect(cv::Mat& img)
{
  std::vector<cv::Rect> faces;
  double current_scale;
  switch(ltr_int_wc_get_optim_level()){
    case 0:
      cv::equalizeHist(img, img);
      cascade->detectMultiScale(img, faces, 1.1, 2, 0, minFace);
      current_scale = 1;
      break;
    case 1:
      cv::equalizeHist(img, img);
      cascade->detectMultiScale(img, faces, 1.2, 2, 0, minFace);
      current_scale = 1;
      break;
    case 2:
      cv::resize(img, scaled, cv::Size(), scale, scale);
      cv::equalizeHist(scaled, scaled);
      cascade->detectMultiScale(scaled, faces, 1.1, 2, 0, minFace);
      current_scale = scale;
      break;
    case 3:
    default:
      cv::resize(img, scaled, cv::Size(), scale, scale);
      cv::equalizeHist(scaled, scaled);
      cascade->detectMultiScale(scaled, faces, 1.2, 2, 0, minFace);
      current_scale = scale;
      break;
  }
  
  double area = -1;
  const cv::Rect *candidate = NULL;
  for(std::vector<cv::Rect>::const_iterator i = faces.begin(); i != faces.end(); ++i){
    if(i->width * i->height > area){
      candidate = &(*i);
      area = i->width * i->height;
    }
  }
  if(candidate != NULL){
    expFiltFactor = ltr_int_wc_get_eff();
    
    float x = (candidate->x + candidate->width / 2) / current_scale - frame_w/2;
    float y = (candidate->y + candidate->height / 2) / current_scale - frame_h/2;
    float w = candidate->width / current_scale;
    float h = candidate->height / current_scale;
    
    static float last_face_x = 0.0f;
    static float last_face_y = 0.0f;
    static float last_face_w = 0.0f;
    static float last_face_h = 0.0f;
    
    if(init){
      last_face_x = face_x = x;
      last_face_y = face_y = y;
      last_face_w = face_w = w;
      last_face_h = face_h = h;
      init = false;
    }else{
      face_x = expfilt(x, last_face_x, expFiltFactor);
      face_y = expfilt(y, last_face_y, expFiltFactor);
      face_w = expfilt(w, last_face_w, expFiltFactor);
      face_h = expfilt(h, last_face_h, expFiltFactor);
      last_face_x = face_x;
      last_face_y = face_y;
      last_face_w = face_w;
      last_face_h = face_h;
    }
  }
//  std::cout<<"Done" <<std::endl;
}

static bool run = true;
static enum {READY, PROCESSING, DONE} frame_status  = DONE;
static bool request_frame = false;
static pthread_cond_t frame_cv = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t frame_mx = PTHREAD_MUTEX_INITIALIZER;
static pthread_t detect_thread_handle;


void *detector_thread(void *)
{
  cascade = new cv::CascadeClassifier();
  const char *cascade_path = ltr_int_wc_get_cascade();
  if(cascade_path == NULL){
    ltr_int_log_message("Cascade path not specified!\n");
    return NULL;
  }
  if(!cascade->load(cascade_path)){
    ltr_int_log_message("Could't load cascade!\n");
    return NULL;
  }
  init = true;
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
    double t = (double)cvGetTickCount();
    detect(*cvimage);
    t = (double)cvGetTickCount() - t;
    //std::cout<<"detection time = "<<t/((double)cvGetTickFrequency()*1000.)<<" ms"<<std::endl;

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
  if(face_w * face_h > 0){
    blt->num_blobs = 1;
    blt->blobs[0].x = face_x;
    blt->blobs[0].y = face_y;
    blt->blobs[0].score = face_w * face_h;
    ltr_int_draw_cross(img, face_x + frame_w/2, face_y + frame_h/2, 20);
  }else{
    blt->num_blobs = 0;
  }
}


float expfilt(float x, 
               float y_minus_1,
               float filterfactor) 
{
  float y;
  
  y = y_minus_1*(1.0-filterfactor) + filterfactor*x;
  return y;
}

