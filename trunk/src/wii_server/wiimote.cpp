
#include "wiimote.h"
#include "../image_process.h"
#include "com_proc.h"
#include <cwiid.h>
#include <iostream>

#define WIIMOTE_HORIZONTAL_RESOLUTION 1024
#define WIIMOTE_VERTICAL_RESOLUTION 768

static struct mmap_s *mm;


void WiiThread::idle()
{
  idle_cntr++;
  if(idle_cntr == 20){
    cwiid_set_led(gWiimote, 0);
  }
  if(idle_cntr > 500){
    idle_cntr = 0;
    cwiid_set_led(gWiimote, idle_sig);
    idle_sig <<= 1;
    if(idle_sig > 16){
      idle_sig = 1;
    }
  }
}

static WiiThread *target_class = NULL;

static void wii_callback(cwiid_wiimote_t *wii, int count, union cwiid_mesg mesg[], struct timespec *time)
{
  (void) wii;
  (void) time;
  (void) mesg;
  int i;
  for(i = 0; i < count; ++i){
    switch(mesg[i].type){
      case CWIID_MESG_STATUS:
        printf("  Status - battery: %d\n", mesg[i].status_mesg.battery);
        break;
      case CWIID_MESG_IR:
        if(target_class != NULL) target_class->pass_ir_data(mesg[i].ir_mesg.src);
        break;
      case CWIID_MESG_ERROR:
        if(mesg[i].error_mesg.error != CWIID_ERROR_NONE){
          printf("Got disconnected!\n");
          if(target_class != NULL) target_class->stop_it();
        }
        break;
      default:
        printf("  Default:\n");
        break;
    }
  }
}


void WiiThread::run()
{
  bdaddr = (bdaddr_t) {{0, 0, 0, 0, 0, 0}};
  idle_cntr = 0;
  idle_sig = 1;
  isIdle = true;
  exit_flag = false;
  emit change_state(WII_CONNECTING);
  std::cout <<"Put Wiimote in discoverable mode now (press 1+2)..." << std::endl;
  if(!(gWiimote = cwiid_open(&bdaddr, CWIID_FLAG_MESG_IFC))){
    std::cout << "Wiimote not found" << std::endl;
    emit change_state(WII_DISCONNECTED);
    return;
  }else{
    cwiid_set_rpt_mode(gWiimote, CWIID_RPT_STATUS);
    target_class = this;
    cwiid_set_mesg_callback(gWiimote, wii_callback);
    std::cout << "Wiimote connected" << std::endl;
    emit change_state(WII_CONNECTED);
  }
  while(!exit_flag){
    if(isIdle) idle();  //take care of idle indication
    
    msleep(8);
  }
  cwiid_set_mesg_callback(gWiimote, NULL);
  cwiid_close(gWiimote);
  gWiimote = NULL;
  emit change_state(WII_DISCONNECTED);
}

void WiiThread::pass_ir_data(struct cwiid_ir_src *data)
{
  int i;
  int valid = 0;
  bool get_frame = ltr_int_getFrameFlag(mm);
  image img;
  img.w = WIIMOTE_HORIZONTAL_RESOLUTION / 2;
  img.h = WIIMOTE_VERTICAL_RESOLUTION / 2;
  img.bitmap = ltr_int_getFramePtr(mm);
  img.ratio = 1.0;
  
  struct blob_type blobs_array[3] = {
    {0.0f, 0.0f, -1},
    {0.0f, 0.0f, -1},
    {0.0f, 0.0f, -1}
  };
  struct bloblist_type bloblist;
  bloblist.num_blobs = 3;
  bloblist.blobs = blobs_array;
    
  if(!get_frame){
    memset(img.bitmap, 0, img.w * img.h);
  }
  
  for(i = 0; i < 4; ++i){
    if((data[i].valid) && (valid < 3)){
      (blobs_array[valid]).x = data[i].pos[0];
      (blobs_array[valid]).y = data[i].pos[1];
      (blobs_array[valid]).score = data[i].size;
      if(!get_frame){
        ltr_int_draw_square(&img, data[i].pos[0] / 2, (WIIMOTE_VERTICAL_RESOLUTION - data[i].pos[1]) / 2,
          2*data[i].size);
        ltr_int_draw_cross(&img, data[i].pos[0] / 2, (WIIMOTE_VERTICAL_RESOLUTION - data[i].pos[1]) / 2, 
          (int)WIIMOTE_HORIZONTAL_RESOLUTION/100.0);
      }
      ++valid;
    }
  }
  ltr_int_setBlobs(mm, blobs_array, bloblist.num_blobs);
  if(!get_frame){
    ltr_int_setFrameFlag(mm);
  }
}


void WiiThread::stop_it()
{
  exit_flag = true;
}

void WiiThread::pause(int indication)
{
  if(gWiimote != NULL){
    cwiid_set_led(gWiimote, indication);
    cwiid_set_rpt_mode(gWiimote, CWIID_RPT_STATUS);
  }
  isIdle = false;
}

void WiiThread::wakeup(int indication)
{
  if(gWiimote != NULL){
    cwiid_set_led(gWiimote, indication);
    cwiid_set_rpt_mode(gWiimote, CWIID_RPT_STATUS | CWIID_RPT_IR);
  }
  isIdle = false;
}

void WiiThread::stop()
{
  if(gWiimote != NULL){
    cwiid_set_rpt_mode(gWiimote, CWIID_RPT_STATUS);
  }
  isIdle = true;
}

Wiimote::Wiimote(struct mmap_s *m) : server_state(WII_DISCONNECTED), thread(NULL)
{
  mm = m;
  thread = new WiiThread(); 
  QObject::connect(thread, SIGNAL(change_state(int)), this, SLOT(changed_state(int)));
  QObject::connect(this, SIGNAL(pause(int)), thread, SLOT(pause(int)));
  QObject::connect(this, SIGNAL(wakeup(int)), thread, SLOT(wakeup(int)));
  QObject::connect(this, SIGNAL(stop()), thread, SLOT(stop()));
}

Wiimote::~Wiimote()
{
  std::cout << "closing wiimote" << std::endl;
  if(thread->isRunning()){
    thread->stop_it();
    thread->wait(1000);
    if(thread->isRunning()){
      thread->terminate();
    }
  }
  delete thread;
}

void Wiimote::connect(void)
{
  if(thread->isRunning()){
    return;
  }else{
    thread->start();
  }
}

void Wiimote::disconnect(void)
{
  if(thread->isRunning()){
    thread->stop_it();
  }
}

void Wiimote::changed_state(int state)
{
  server_state = (server_state_t)state;
  emit changing_state(server_state);
}


