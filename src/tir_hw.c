#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <zlib.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include "tir_hw.h"
#include "tir_img.h"
#include "usb_ifc.h"
#include "utils.h"
#include "tir_driver_prefs.h"
#include "tir.h"
#include "ipc_utils.h"


#define TIR_CONFIGURATION 1
#define TIR_INTERFACE 0
#define TIR_OUT_EP  0x01
#define TIR3_OUT_EP  0x02
#define TIR_IN_EP 0x82
#define SN4_OUT_EP 0x02
#define SN4_DATA_EP 0x86
#define SN4_CFG_EP 0x84

#define FW_SIZE_INCREMENT 50000

//Known packets
static unsigned char Video_off[] = {0x14, 0x01};
static unsigned char Video_on[] = {0x14, 0x00};
static unsigned char Fifo_flush[] = {0x12};
static unsigned char Camera_stop[] = {0x13};
static unsigned char Fpga_init[] = {0x1b};
static unsigned char Cfg_reload[] = {0x20};
static unsigned char Get_status[] = {0x1d};
static unsigned char Get_conf[] = {0x17};
static unsigned char Precision_mode[] = {0x19, 0x03, 0x10, 0x00, 0x05};
static unsigned char Set_ir_brightness[] =  {0x10, 0x00, 0x02, 0x00, 0xA0};
static unsigned char SN4_normal_mode[] = {0x19, 0x03, 0x0F, 0x00, 0x00};
static unsigned char SN4_grayscale_mode[] = {0x19, 0x03, 0x0F, 0x00, 0x04};


//Unknown packets
static unsigned char unk_1[] =  {0x17, 0x01};
static unsigned char unk_2[] =  {0x12, 0x01};
static unsigned char unk_3[] =  {0x13, 0x01};
static unsigned char unk_4[] =  {0x19, 0x15, 0x10, 0x40, 0x00};
static unsigned char unk_5[] =  {0x23, 0x40, 0x1c, 0x5e, 0x00, 0x00};
static unsigned char unk_6[] =  {0x23, 0x40, 0x1d, 0x01, 0x00, 0x00};
static unsigned char unk_7[] =  {0x19, 0x05, 0x10, 0x10, 0x00};
static unsigned char unk_8[] =  {0x19, 0x03, 0x03, 0x00, 0x00};

//SmartNav4 init
static unsigned char unk_9[] =  {0x19, 0x04, 0x0F, 0x00, 0x0F};
static unsigned char unk_a[] =  {0x19, 0x14, 0x10, 0x00, 0x01};
static unsigned char unk_b[] =  {0x23, 0x90, 0x0B, 0x00, 0x01, 0x3C};
static unsigned char unk_c[] =  {0x23, 0x90, 0xF0, 0x32, 0x01, 0x3C};
static unsigned char unk_d[] =  {0x19, 0x14, 0x10, 0x00, 0x00};
static unsigned char unk_e[] =  {0x1F, 0x20};

static bool ir_on = true;

static dev_found device = NOT_TIR;

static tir_interface tir2;
static tir_interface tir3;
static tir_interface tir4;
static tir_interface tir5;
static tir_interface smartnav3;
static tir_interface smartnav4;
static tir_interface tir5v3;
static tir_interface *tir_iface = NULL;

static const char *sn4_pipe_name = "ltr_sn4.pipe";
static int sn4_pipe = -1;

static int cfg_in_ep;
int ltr_int_data_in_ep;
static int out_ep;

typedef struct{
  bool fw_loaded;
  int cfg_flag;
  unsigned int fw_cksum;
} tir_status_t;

typedef struct{
  unsigned char *firmware;
  size_t size;
  unsigned int cksum;
} firmware_t;


unsigned char ltr_int_packet[TIR_PACKET_SIZE];
static void switch_red(bool state);
static void switch_green(bool state);

static bool ltr_int_open_sn4_pipe()
{
  if(sn4_pipe > 0){
    return true;
  }
  char *fname = ltr_int_get_default_file_name(sn4_pipe_name);
  sn4_pipe = ltr_int_create_server_socket(fname);
  free(fname);
  return(sn4_pipe > 0);
}

void ltr_int_send_sn4_data(uint8_t data[], size_t length)
{
  if(sn4_pipe <= 0){
    printf("Pipe not open, trying to open...\n");
    ltr_int_open_sn4_pipe();
  }
  if(sn4_pipe <= 0){
    printf("There was problem opening the pipe!\n");
    return;
  }
  ltr_int_socket_send(sn4_pipe, (void *)data, length);
}

static void ltr_int_close_sn4_pipe()
{
  if(sn4_pipe <= 0){
    return;
  }
  ltr_int_close_socket(sn4_pipe);
  unlink(ltr_int_get_default_file_name(sn4_pipe_name));
  sn4_pipe = -1;
  ltr_int_log_message("SN4 pipe closed!\n");
}

/*
static void set_status_brightness_tir(unsigned char b)
{
  if(b > 3){
    b = 3;
  }
  status_brightness = b;
}

static void set_ir_brightness_tir(unsigned char b)
{
  if(b < 5){
    b = 5;
  }
  if(b > 7){
    b = 7;
  }
  ir_brightness = b;
}
*/


static void cksum_firmware(firmware_t *fw)
{
  assert(fw != NULL);
  unsigned int cksum = 0;
  unsigned int byte = 0;

  unsigned char *firmware = fw->firmware;
  size_t size = fw->size;

  while(size > 0){
    byte = (unsigned int)*firmware;

    cksum += byte;
    byte = byte << 4;
    cksum ^= byte;

    --size;
    ++firmware;
  }
  fw->cksum = cksum & 0xffff;
}


static int time_diff_msec(struct timeval *tv1, struct timeval *tv0)
{
  int res = (int)(1000.0 * difftime(tv1->tv_sec, tv0->tv_sec) + (tv1->tv_usec - tv0->tv_usec) / 1000.0);
  return res;
}


static bool read_status_tir(tir_status_t *status)
{
  assert(status != NULL);
  size_t t;
  int counter = 0;
  struct timeval tv1, tv2;
  struct timezone tz;
  gettimeofday(&tv1, &tz);

  while(counter < 20){
    ltr_int_log_message("Requesting status.\n");
    if(!ltr_int_send_data(out_ep, Get_status, sizeof(Get_status))){
      ltr_int_log_message("Couldn't send status request!\n");
      return false;
    }

    while(1){
      if(!ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 500)){
        ltr_int_log_message("Couldn't receive status!\n");
        return false;
      }
      if((t > 2) && (ltr_int_packet[0] == 0x07) && (ltr_int_packet[1] == 0x20)){
        break;
      }
      gettimeofday(&tv2, &tz);
      if(time_diff_msec(&tv2, &tv1) > 100){
        ltr_int_log_message("Status request timed out, will try again...\n");
        tv1 = tv2;
        break;
      }
    }
    if((t > 2) && (ltr_int_packet[0] == 0x07) && (ltr_int_packet[1] == 0x20)){
      break;
    }
    counter++;
  }
  ltr_int_log_message("Status packet: %02X %02X %02X %02X %02X %02X %02X\n",
    ltr_int_packet[0], ltr_int_packet[1], ltr_int_packet[2], ltr_int_packet[3],
    ltr_int_packet[4], ltr_int_packet[5], ltr_int_packet[6]);

  status->fw_loaded = (ltr_int_packet[3] == 1) ? true : false;
  status->cfg_flag = ltr_int_packet[6];
  status->fw_cksum = (((unsigned int)ltr_int_packet[4]) << 8) + (unsigned int)ltr_int_packet[5];
  return true;
}

static bool read_rom_data_tir3()
{
  size_t t;
  int counter = 0;
  struct timeval tv1, tv2;
  struct timezone tz;
  while(counter < 20){
    if(!ltr_int_send_data(out_ep, unk_1, sizeof(unk_1))){
      ltr_int_log_message("Couldn't send config data request!\n");
      return false;
    }
    ltr_int_usleep(6000);
    gettimeofday(&tv1, &tz);
    while(1){
      if(!ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 1000)){
        ltr_int_log_message("Couldn't receive status!\n");
        return false;
      }
      if((ltr_int_packet[0] == 0x09) && (ltr_int_packet[1] == 0x40)){
        break;
      }
      gettimeofday(&tv2, &tz);
      if(time_diff_msec(&tv2, &tv1) > 100){
        ltr_int_log_message("Status request timed out, will try again...\n");
        tv1 = tv2;
        break;
      }
    }
    if((ltr_int_packet[0] == 0x09) && (ltr_int_packet[1] == 0x40)){
      break;
    }
    counter++;
  }
  return true;
}


static bool read_rom_data_tir()
{
  size_t t;
  ltr_int_log_message("Flushing packets...\n");
  do{
    ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  }while(t > 0);
  ltr_int_log_message("Sending get_conf request.\n");
  int counter = 0;
  struct timeval tv1, tv2;
  struct timezone tz;
  gettimeofday(&tv1, &tz);
  while(counter < 10){
    if(!ltr_int_send_data(out_ep, Get_conf, sizeof(Get_conf))){
      ltr_int_log_message("Couldn't send config data request!\n");
      return false;
    }

    while(1){
      ltr_int_log_message("Requesting data...\n");
      if(!ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 500)){
        ltr_int_log_message("Couldn't receive status!\n");
        return false;
      }
      //Tir4/5 0x14, Tir3 0x09, SN4 0x15...
      if((t > 2) && (ltr_int_packet[1] == 0x40)){
        break;
      }
      gettimeofday(&tv2, &tz);
      if(time_diff_msec(&tv2, &tv1) > 100){
        ltr_int_log_message("Request for data timed out. Will try later...\n");
        tv1 = tv2;
        break;
      }
    }
    //Tir4/5 0x14, Tir3 0x09, SN4 0x15...
    if((t > 2) && (ltr_int_packet[1] == 0x40)){
      break;
    }
    counter++;
  }
  return true;
}



static bool load_firmware(firmware_t *fw)
{
  assert(fw != NULL);
  gzFile f;
  unsigned int tsize = FW_SIZE_INCREMENT;
  unsigned char *tbuf = ltr_int_my_malloc(tsize);
  unsigned char *ptr = tbuf;
  unsigned int bytesRead = 0;
  size_t *size = &(fw->size);
  *size = 0;

  char *fw_path = ltr_int_find_firmware(device);
  ltr_int_log_message("Loading firmware '%s'\n", fw_path);
  f = gzopen(fw_path, "rb");
  if(f == NULL){
    ltr_int_log_message("Couldn't open firmware (%s)!\n", fw_path);
    free(fw_path);
    return false;
  }
  free(fw_path);

  while(1){
    bytesRead = gzread(f, ptr, FW_SIZE_INCREMENT);
    *size += bytesRead;
    if(bytesRead < FW_SIZE_INCREMENT){
      break;
    }
    tsize += FW_SIZE_INCREMENT;
    if((tbuf = realloc(tbuf, tsize)) == NULL){
      break;
    }
    ptr = tbuf + *size; //realloc can move data!!!
  }

  gzclose(f);
  if(tbuf != NULL){
    fw->firmware = tbuf;
    cksum_firmware(fw);
    ltr_int_log_message("Size: %d  Cksum: %04X\n", fw->size, fw->cksum);
    return true;
  }else{
    fw->firmware = NULL;
    return false;
  }
}


static bool upload_firmware(firmware_t *fw)
{
  unsigned char buf[62];
  unsigned char len;
  unsigned char *firmware = fw->firmware;
  size_t size = fw->size;

  if(!ltr_int_send_data(out_ep, Fpga_init, sizeof(Fpga_init))){
    ltr_int_log_message("Couldn't init fpga!\n");
    return false;
  }

  buf[0] = 0x1c;
  while(size>0){
    len = (size > 60) ? 60 : size;
    size -= len;
    buf[1] = len;
    memcpy(&(buf[2]), firmware, len);
    firmware += len;

    if(!ltr_int_send_data(out_ep, buf, len + 2)){
      ltr_int_log_message("Couldn't send data!\n");
      return false;
    }
  }
  ltr_int_log_message("Firmware uploaded!\n");

  return true;
}

static bool wiggle_leds_tir(unsigned char leds, unsigned char mask)
{
  unsigned char msg[3] = {0x10, leds, mask};
  if(!ltr_int_send_data(out_ep, msg, sizeof(msg))){
    ltr_int_log_message("Problem wiggling LEDs\n");
    return false;
  }
  ltr_int_usleep(2000);
  return true;
}

static bool turn_led_off_tir(unsigned char leds)
{
  leds &= 0xf0;
  return wiggle_leds_tir(0, leds);
}

static bool turn_led_on_tir(unsigned char leds)
{
  leds &= 0xf0;
  return wiggle_leds_tir(leds, leds);
}

static bool flush_fifo_tir()
{
  if((device == TIR5)||(device == TIR5V2)){
    ltr_int_usleep(100000);
  }
  ltr_int_log_message("Flushing FIFO.\n");
  return ltr_int_send_data(out_ep, Fifo_flush,sizeof(Fifo_flush));
}

static bool set_threshold_tir5v3(unsigned int t);

bool ltr_int_set_threshold_tir(unsigned int val)
{
  if(device == TIR5V3){
    return set_threshold_tir5v3(val);
  }
  unsigned char pkt[] = {0x15, 0x96, 0x01, 0x00};
  size_t pkt_len = sizeof(pkt);
  if(val > 253){
    val = 253;
  }
  if(device > TIR2){
    if(val < 30){
      val = 30;
    }
  }else{
    if(val < 40){
      val = 40;
    }
    pkt_len -= 1;
  }
  pkt[1] = val;
  ltr_int_log_message("Setting threshold.\n");
  return ltr_int_send_data(out_ep, pkt, pkt_len);
}

static bool set_status_led_tir5(bool running)
{
  unsigned char pkt[] =  {0x19, 0x04, 0x10, 0x00, 0x00};
  pkt[3] = ltr_int_tir_get_status_brightness();
  if(running){
    pkt[4] = 0x22;
  }else{
    pkt[4] = 0x33;
  }
  return ltr_int_send_data(out_ep, pkt, sizeof(pkt));
}

static bool set_status_led_tir4(bool running)
{
  if(running){
    switch_red(false);
    switch_green(true);
  }else{
    switch_green(false);
    switch_red(true);
  }
  return true;
}

static bool set_status_led_sn4(bool running)
{
  if(running){
    switch_red(false);
    switch_green(true);
  }else{
    switch_green(false);
    switch_red(true);
  }
  return true;
}

static bool set_status_led_sn3(bool running)
{
  if(running){
    switch_green(true);
  }else{
    switch_green(false);
  }
  return true;
}

bool set_status_led_tir(bool running)
{
  assert(tir_iface != NULL);
  if(ltr_int_tir_get_status_indication()){
    return tir_iface->set_status_led_tir(running);
  }
  return true;
}

static bool control_status_led_tir(bool status1, bool status2)
{
  unsigned char pkt[] =  {0x19, 0x04, 0x10, 0x00, 0x00};
  ltr_int_log_message("Setting status LED.\n");
  pkt[3] = ltr_int_tir_get_status_brightness();
  pkt[4] = (status1 ? 0x20 : 0) | (status2 ? 0x02 : 0);
  return ltr_int_send_data(out_ep, pkt, sizeof(pkt));
}

static bool turn_off_status_led_tir5()
{
  unsigned char pkt[] =  {0x19, 0x04, 0x10, 0x00, 0x00};
  ltr_int_log_message("Turning status LED off (TIR5).\n");
  return ltr_int_send_data(out_ep, pkt, sizeof(pkt));
}

static bool control_ir_led_tir(bool ir)
{
  unsigned char pkt[] =  {0x19, 0x09, 0x10, 0x00, 0x00};
  ltr_int_log_message("Setting IR LED.\n");
  int ir_brightness = ltr_int_tir_get_ir_brightness();
  pkt[3] = ir ? ir_brightness : 0;
  pkt[4] = ir ? 0x01 : 0;
  return ltr_int_send_data(out_ep, pkt, sizeof(pkt));
}

static bool set_exposure(unsigned int exp)
{
  unsigned char Set_exposure_h[] =  {0x23, 0x42, 0x08, 0x01, 0x00, 0x00};
  unsigned char Set_exposure_l[] =  {0x23, 0x42, 0x10, 0x8F, 0x00, 0x00};

  Set_exposure_h[3] = exp >> 8;
  Set_exposure_l[3] = exp & 0xFF;
  ltr_int_log_message("Setting exposure.\n");
  return ltr_int_send_data(out_ep, Set_exposure_h, sizeof(Set_exposure_h)) &&
    ltr_int_send_data(out_ep, Set_exposure_l, sizeof(Set_exposure_l));

}

static bool stop_camera_tir2()
{
  ltr_int_log_message("Stopping TIR2 camera!\n");
  turn_led_off_tir(TIR_LED_RED);
  ltr_int_usleep(70000);
  turn_led_off_tir(TIR_LED_GREEN);
  ltr_int_usleep(70000);
  turn_led_off_tir(TIR_LED_BLUE);
  ltr_int_usleep(70000);
  turn_led_off_tir(TIR_LED_IR);
  ltr_int_usleep(70000);
  ltr_int_send_data(out_ep, Camera_stop,sizeof(Camera_stop));
  ltr_int_usleep(70000);
  turn_led_off_tir(TIR_LED_IR);
  return true;
}


static bool stop_camera_tir3()
{
  ltr_int_log_message("Stopping TIR3 camera!\n");
  ltr_int_send_data(out_ep, Video_off,sizeof(Video_off));
  ltr_int_usleep(2000);
  ltr_int_send_data(out_ep, Video_off,sizeof(Video_off));
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_IR);
  ltr_int_usleep(2000);
  flush_fifo_tir();
  ltr_int_usleep(2000);
  ltr_int_send_data(out_ep, Camera_stop,sizeof(Camera_stop));
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_RED);
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_GREEN);
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_BLUE);
  ltr_int_usleep(50000);
  return true;
}

static bool stop_camera_tir4()
{
  ltr_int_log_message("Stopping the TIR4 camera.\n");
  ltr_int_send_data(out_ep, Video_off,sizeof(Video_off));
  flush_fifo_tir();
  turn_led_off_tir(TIR_LED_IR);
  turn_led_off_tir(TIR_LED_RED);
  turn_led_off_tir(TIR_LED_GREEN);
  turn_led_off_tir(TIR_LED_BLUE);
  ltr_int_log_message("Sending stop packet to TIR4 camera.\n");
  ltr_int_send_data(out_ep, Camera_stop,sizeof(Camera_stop));
  ltr_int_usleep(50000);
  return true;
}

static bool stop_camera_tir5()
{
  turn_off_status_led_tir5();
  control_ir_led_tir(false);
  ltr_int_usleep(50000);
  ltr_int_send_data(out_ep, Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(out_ep, Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(out_ep, Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(out_ep, Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(out_ep, Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(out_ep, unk_2,sizeof(unk_2));
  ltr_int_send_data(out_ep, unk_3,sizeof(unk_3));
  ltr_int_send_data(out_ep, Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(out_ep, Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(out_ep, Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(out_ep, Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(out_ep, Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  flush_fifo_tir();
  ltr_int_send_data(out_ep, Camera_stop,sizeof(Camera_stop));
  turn_off_status_led_tir5();
  ltr_int_usleep(50000);
  turn_off_status_led_tir5();
  control_ir_led_tir(false);
  turn_led_off_tir(TIR_LED_IR);
  return true;
}

static bool stop_camera_sn4()
{
  ltr_int_log_message("Stopping SmartNav4 camera!\n");
  ltr_int_send_data(out_ep, Video_off, sizeof(Video_off));
  turn_led_off_tir(TIR_LED_IR);
  turn_led_off_tir(TIR_LED_RED);
  turn_led_off_tir(TIR_LED_GREEN);
  turn_led_off_tir(TIR_LED_BLUE);
  ltr_int_log_message("Sending stop packet to SmartNav4 camera.\n");
  ltr_int_send_data(out_ep, Camera_stop,sizeof(Camera_stop));
  ltr_int_usleep(50000);
  return true;
}

static bool stop_camera_sn3()
{
  ltr_int_log_message("Stopping SmartNav3 camera!\n");
  ltr_int_send_data(out_ep, Video_off, sizeof(Video_off));
  ltr_int_send_data(out_ep, Fifo_flush, sizeof(Fifo_flush));

  turn_led_off_tir(TIR_LED_GREEN);
  turn_led_off_tir(TIR_LED_IR);
  turn_led_off_tir(TIR_LED_RED);
  turn_led_off_tir(TIR_LED_BLUE);
  ltr_int_log_message("Sending stop packet to SmartNav3 camera.\n");
  ltr_int_send_data(out_ep, Camera_stop,sizeof(Camera_stop));
  return true;
}

static bool stop_camera_tir()
{
  assert(tir_iface != NULL);
  return tir_iface->stop_camera_tir();
}

static bool start_camera_tir2()
{
  ltr_int_log_message("Starting TIR2 camera!\n");
  ltr_int_send_data(out_ep, Video_on,sizeof(Video_on));
  ltr_int_usleep(120000);
  if(ir_on){
    turn_led_on_tir(TIR_LED_IR);
    ltr_int_usleep(64000);
  }
  flush_fifo_tir();
  ltr_int_usleep(64000);
  if(ltr_int_tir_get_status_indication()){
    set_status_led_tir4(true);
    ltr_int_usleep(64000);
  }
  if(ir_on){
    turn_led_on_tir(TIR_LED_IR);
    ltr_int_usleep(64000);
  }
  return true;
}

static bool start_camera_tir3()
{
  ltr_int_log_message("Starting TIR3 camera!\n");
  flush_fifo_tir();
  ltr_int_usleep(2000);
  ltr_int_send_data(out_ep, Camera_stop,sizeof(Camera_stop));
  ltr_int_usleep(2000);
  if(ir_on){
    turn_led_on_tir(TIR_LED_IR);
    ltr_int_usleep(2000);
  }
  ltr_int_send_data(out_ep, Video_on,sizeof(Video_on));
  ltr_int_usleep(2000);
  if(ltr_int_tir_get_status_indication()){
    set_status_led_tir4(true);
    ltr_int_usleep(2000);
  }
  return true;
}

static bool start_camera_tir4()
{
  stop_camera_tir();
  ltr_int_send_data(out_ep, Video_on,sizeof(Video_on));
  if(ir_on){
    turn_led_on_tir(TIR_LED_IR);
  }
  flush_fifo_tir();
  if(ltr_int_tir_get_status_indication())
    set_status_led_tir4(true);
  return true;
}

static bool start_camera_tir5()
{
  stop_camera_tir();
  ltr_int_send_data(out_ep, Video_on,sizeof(Video_on));
  if(ir_on){
    control_ir_led_tir(true);
  }else{
    control_ir_led_tir(false);
  }
  if(ltr_int_tir_get_status_indication())
    set_status_led_tir5(true);
  return true;
}

static bool start_camera_sn4()
{
  ltr_int_log_message("Starting SmartNav4 camera!\n");
  if(ltr_int_tir_get_use_grayscale()){
    ltr_int_send_data(out_ep, SN4_grayscale_mode,sizeof(SN4_grayscale_mode));
  }else{
    ltr_int_send_data(out_ep, SN4_normal_mode,sizeof(SN4_normal_mode));
  }
  ltr_int_send_data(out_ep, Video_on,sizeof(Video_on));
  if(ir_on){
    turn_led_on_tir(TIR_LED_IR);
  }
  if(ltr_int_tir_get_status_indication()){
    set_status_led_tir4(true);
  }
  return true;
  return true;
}

static bool start_camera_sn3()
{
  ltr_int_send_data(out_ep, Video_on,sizeof(Video_on));
  ltr_int_send_data(out_ep, Fifo_flush,sizeof(Fifo_flush));
  turn_led_on_tir(TIR_LED_GREEN);
  if(ir_on){
    turn_led_on_tir(TIR_LED_IR);
  }
  turn_led_on_tir(TIR_LED_BLUE);
  return true;
}

bool start_camera_tir()
{
  assert(tir_iface != NULL);
  return tir_iface->start_camera_tir();
}

static bool init_camera_tir2(bool force_fw_load, bool p_ir_on)
{
  (void) force_fw_load;
  size_t t;

  ir_on = p_ir_on;
  ltr_int_log_message("Initializing TIR2 camera!\n");
  turn_led_off_tir(TIR_LED_IR);
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_RED);
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_GREEN);
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_BLUE);
  ltr_int_usleep(2000);
  ltr_int_send_data(out_ep, Video_off,sizeof(Video_off));
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_IR);
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_RED);
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_GREEN);
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_BLUE);
  ltr_int_usleep(2000);
  //To flush any pending packets...
  ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  if(!read_rom_data_tir3()){
    return false;
  }
  ltr_int_usleep(70000);
  ltr_int_set_threshold_tir(0x8c);
  ltr_int_usleep(2000);
  return true;
}

static bool init_camera_tir3(bool force_fw_load, bool p_ir_on)
{
  (void) force_fw_load;
  size_t t;

  ir_on = p_ir_on;
  ltr_int_log_message("Initializing TIR3 camera!\n");
  if(!stop_camera_tir()){
    return false;
  }
  //To flush any pending packets...
  ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  if(!read_rom_data_tir3()){
    return false;
  }
  ltr_int_usleep(70000);
  ltr_int_set_threshold_tir(0x8c);
  ltr_int_usleep(70000);
  ltr_int_send_data(out_ep, unk_5,sizeof(unk_5));
  ltr_int_usleep(2000);
  ltr_int_send_data(out_ep, unk_6,sizeof(unk_6));
  ltr_int_usleep(2000);
  ltr_int_set_threshold_tir(0xd0);
  ltr_int_usleep(2000);
  ltr_int_send_data(out_ep, unk_8,sizeof(unk_8));
  ltr_int_usleep(2000);
  return true;
}


static bool init_camera_tir4(bool force_fw_load, bool p_ir_on)
{
  tir_status_t status;
  size_t t;

  ir_on = p_ir_on;

  if(!stop_camera_tir()){
    return false;
  }
  //To flush any pending packets...
  ltr_int_log_message("Flushing packets...\n");
  do{
    ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  }while(t > 0);
  ltr_int_log_message("Packets flushed.\n");
  if(!read_rom_data_tir()){
    return false;
  }
  if(!read_status_tir(&status)){
    return false;
  }
  firmware_t firmware;
  if(!load_firmware(&firmware)){
    ltr_int_log_message("Error loading firmware!\n");
    return false;
  }

  if(force_fw_load | (!status.fw_loaded) | (status.fw_cksum != firmware.cksum)){
    upload_firmware(&firmware);
  }

  if(!read_status_tir(&status)){
    ltr_int_log_message("Couldn't retrieve status!\n");
    return false;
  }

  if(status.fw_cksum != firmware.cksum){
    ltr_int_log_message("Firmware not loaded correctly!\n");
    return false;
  }

  if(status.cfg_flag == 1){
    ltr_int_send_data(out_ep, Cfg_reload,sizeof(Cfg_reload));
    while(status.cfg_flag != 2){
      if(!read_status_tir(&status)){
	return false;
      }
    }
    if((device == TIR5)||(device == TIR5V2)){
      ltr_int_send_data(out_ep, Set_ir_brightness,sizeof(Set_ir_brightness));
      set_exposure(0x18F);
    }
    ltr_int_set_threshold_tir(0x82);
  }else if(status.cfg_flag != 2){
    ltr_int_log_message("TIR configuration problem!\n");
    return false;
  }


  if((device == TIR5)||(device == TIR5V2)){
    ltr_int_send_data(out_ep, Precision_mode,sizeof(Precision_mode));
  }

  free(firmware.firmware);
  ltr_int_log_message("TIR4 camera initialized.\n");
  return true;
}



static bool init_camera_tir5(bool force_fw_load, bool p_ir_on)
{
  (void) force_fw_load;
  tir_status_t status;
  ir_on = p_ir_on;

  stop_camera_tir();

  read_rom_data_tir();
  control_ir_led_tir(true);
  flush_fifo_tir();
  set_exposure(0x18F);
  read_status_tir(&status);
  firmware_t firmware;
  if(!load_firmware(&firmware)){
    ltr_int_log_message("Error loading firmware!\n");
    return false;
  }
  upload_firmware(&firmware);
  free(firmware.firmware);
  ltr_int_send_data(out_ep, unk_7,sizeof(unk_7));
  flush_fifo_tir();
  ltr_int_send_data(out_ep, Camera_stop,sizeof(Camera_stop));
  read_status_tir(&status);
  ltr_int_send_data(out_ep, Cfg_reload,sizeof(Cfg_reload));
  ltr_int_set_threshold_tir(0x96);
  set_exposure(0x18F);
  control_ir_led_tir(true);
  control_status_led_tir(false, false);
  control_status_led_tir(false, false);
  ltr_int_send_data(out_ep, Precision_mode,sizeof(Precision_mode));
  control_status_led_tir(false, false);
  control_ir_led_tir(true);
  ltr_int_set_threshold_tir(0x76);
  set_exposure(0x15E);
  control_status_led_tir(false, false);
  ltr_int_send_data(out_ep, Precision_mode,sizeof(Precision_mode));
  flush_fifo_tir();
  ltr_int_send_data(out_ep, Camera_stop,sizeof(Camera_stop));
  control_ir_led_tir(true);
  ltr_int_send_data(out_ep, Video_on,sizeof(Video_on));
  return true;
}

static bool init_camera_sn4(bool force_fw_load, bool p_ir_on)
{
  ltr_int_log_message("Initializing SmartNav4...\n");
  tir_status_t status;
  size_t t;

  ir_on = p_ir_on;

  if(!stop_camera_tir()){
    return false;
  }
  //To flush any pending packets...
  ltr_int_log_message("Flushing packets...\n");
  do{
    ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  }while(t > 0);
  ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  ltr_int_log_message("Packets flushed.\n");
  if(!read_rom_data_tir()){
    return false;
  }
  ltr_int_log_message("Flushing packets...\n");
  do{
    ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  }while(t > 0);

  if(!read_status_tir(&status)){
    return false;
  }
  firmware_t firmware;
  if(!load_firmware(&firmware)){
    ltr_int_log_message("Error loading firmware!\n");
    return false;
  }
  if(force_fw_load | (!status.fw_loaded) | (status.fw_cksum != firmware.cksum)){
    upload_firmware(&firmware);
    ltr_int_send_data(out_ep, unk_7,sizeof(unk_7));
    ltr_int_send_data(out_ep, unk_e,sizeof(unk_e));

    ltr_int_log_message("Flushing packets...\n");
    do{
      ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
    }while(t > 0);
    if(!read_status_tir(&status)){
      ltr_int_log_message("Couldn't retrieve status!\n");
      return false;
    }

    if(status.fw_cksum != firmware.cksum){
      ltr_int_log_message("Firmware not loaded correctly!\n");
      return false;
    }

    if(status.cfg_flag == 1){
      ltr_int_send_data(out_ep, Cfg_reload,sizeof(Cfg_reload));
      while(status.cfg_flag != 2){
        if(!read_status_tir(&status)){
          return false;
        }
      }
      ltr_int_send_data(out_ep, unk_9, sizeof(unk_9));
      ltr_int_send_data(out_ep, unk_a, sizeof(unk_a));
      ltr_int_send_data(out_ep, unk_b, sizeof(unk_b));
      ltr_int_send_data(out_ep, unk_c, sizeof(unk_c));
      ltr_int_send_data(out_ep, unk_d, sizeof(unk_d));
      ltr_int_set_threshold_tir(0x78);

    }else if(status.cfg_flag != 2){
      ltr_int_log_message("SmatrNav4 configuration problem!\n");
      return false;
    }

    free(firmware.firmware);
  }
  if(ltr_int_open_sn4_pipe()){
    ltr_int_log_message("SN4 fifo opened successfully!\n");
  }else{
    ltr_int_log_message("Couldn't open SN4 pipe - mouse clicks will not be available!\n");
  }

  ltr_int_log_message("SmartNav4 camera initialized.\n");
  return true;
}

static bool init_camera_sn3(bool force_fw_load, bool p_ir_on)
{
  (void) force_fw_load;
  size_t t;

  ir_on = p_ir_on;
  ltr_int_log_message("Initializing SN3 camera!\n");
  if(!stop_camera_tir()){
    return false;
  }
  //To flush any pending packets...
  ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  if(!read_rom_data_tir()){
    return false;
  }
  return true;
}

bool init_camera_tir(bool force_fw_load, bool p_ir_on)
{
  assert(tir_iface != NULL);
  return tir_iface->init_camera_tir(force_fw_load, p_ir_on);
}

bool ltr_int_open_tir(bool force_fw_load, bool switch_ir_on)
{
  if(!ltr_int_init_usb()){
    ltr_int_log_message("Init failed!\n");
    return false;
  }
  if((device = ltr_int_find_tir()) == NOT_TIR){
    ltr_int_log_message("Tir not found!\n");
    return false;
  }

  //Tir3 uses endpoint 2, while Tir4 and 5 use endpoint 1!
  if((device <= TIR3) || (device == SMARTNAV3)){
    cfg_in_ep = ltr_int_data_in_ep = TIR_IN_EP;
    out_ep = TIR3_OUT_EP;
  }else if(device == SMARTNAV4){
    cfg_in_ep = SN4_CFG_EP;
    ltr_int_data_in_ep = SN4_DATA_EP;
    out_ep = SN4_OUT_EP;
  }else{ //(device == TIR4) || (device == TIR5)
    cfg_in_ep = ltr_int_data_in_ep = TIR_IN_EP;
    out_ep = TIR_OUT_EP;
  }


  if(!ltr_int_prepare_device(TIR_CONFIGURATION, TIR_INTERFACE)){
    ltr_int_log_message("Couldn't prepare!\n");
    return false;
  }
  ltr_int_log_message("Device %d.\n", device);
  switch(device){
    case TIR2:
      tir_iface = &tir2;
      break;
    case TIR3:
      tir_iface = &tir3;
      break;
    case TIR4:
      tir_iface = &tir4;
      break;
    case TIR5:
    case TIR5V2:
      tir_iface = &tir5;
      break;
    case TIR5V3:
      ltr_int_log_message("Initializing TrackIR 5 revision 3.\n");
      tir_iface = &tir5v3;
      break;
    case SMARTNAV4:
      tir_iface = &smartnav4;
      break;
    case SMARTNAV3:
      tir_iface = &smartnav3;
      break;
    default:
      ltr_int_log_message("No device!\n");
      return false;
      break;
  }
  if(!init_camera_tir(force_fw_load, switch_ir_on)){
    return false;
  }
  ltr_int_log_message("Going to start camera.\n");
  start_camera_tir();
  return true;
}

bool ltr_int_pause_tir()
{
  bool res = stop_camera_tir();
  set_status_led_tir(false);
  return res;
}

bool ltr_int_resume_tir()
{
  bool res = start_camera_tir();
  set_status_led_tir(true);
  return res;
}

static bool close_camera_tir2(){
  ltr_int_log_message("Closing TIR2 camera!\n");
  stop_camera_tir();
  ltr_int_usleep(25000);
  turn_led_off_tir(TIR_LED_RED);
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_GREEN);
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_BLUE);
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_IR);
  ltr_int_usleep(2000);
  ltr_int_finish_usb(TIR_INTERFACE);
  ltr_int_log_message("TIR2 camera closed.\n");
  return true;
}

static bool close_camera_tir3(){
  ltr_int_log_message("Closing TIR3 camera!\n");
  stop_camera_tir();
  ltr_int_send_data(out_ep, unk_4,sizeof(unk_4));
  ltr_int_usleep(25000);
  turn_led_off_tir(TIR_LED_RED);
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_GREEN);
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_BLUE);
  ltr_int_usleep(2000);
  ltr_int_finish_usb(TIR_INTERFACE);
  ltr_int_log_message("TIR3 camera closed.\n");
  return true;
}

static bool close_camera_tir4(){
  ltr_int_log_message("Closing the TIR4 camera.\n");
  stop_camera_tir();
  ltr_int_finish_usb(TIR_INTERFACE);
  ltr_int_log_message("TIR4 camera closed.\n");
  return true;
}

static bool close_camera_tir5()
{
  ltr_int_log_message("Closing the TIR5 camera.\n");
  stop_camera_tir();
  ltr_int_set_threshold_tir(0x96);
  set_exposure(0x18F);
  control_ir_led_tir(true);
  control_status_led_tir(false, false);
  control_status_led_tir(false, false);
  ltr_int_send_data(out_ep, Precision_mode,sizeof(Precision_mode));
  turn_led_off_tir(TIR_LED_IR);
  if(!ltr_int_send_data(out_ep, Fpga_init, sizeof(Fpga_init))){
    ltr_int_log_message("Couldn't init fpga!\n");
  }
  ltr_int_finish_usb(TIR_INTERFACE);
  ltr_int_log_message("TIR5 camera closed.\n");
  return true;
}

static bool close_camera_sn4()
{
  stop_camera_tir();
  ltr_int_log_message("Closing the SmartNav4 camera.\n");
  ltr_int_finish_usb(TIR_INTERFACE);
  ltr_int_close_sn4_pipe();
  sn4_pipe = -1;
  ltr_int_log_message("SmartNav4 camera closed.\n");
  return true;
}



bool ltr_int_close_tir()
{
  assert(tir_iface != NULL);
  return tir_iface->close_camera_tir();
}


static void get_tir2_info(tir_info *info)
{
  info->width = 256;
  info->height = 256;
  info->hf = 1.0f;
  info->dev_type = TIR2;
}

static void get_tir3_info(tir_info *info)
{
  info->width = 440;
  info->height = 314;
  info->hf = 1.0f;
  info->dev_type = TIR3;
}

static void get_tir4_info(tir_info *info)
{
  info->width = 400;
  info->height = 300;
  info->hf = 2.0f;
  info->dev_type = TIR4;
}

static void get_tir5_info(tir_info *info)
{
  info->width = 640;
  info->height = 480;
  info->hf = 1.0f;
  info->dev_type = TIR5;
}

static void get_tir5v3_info(tir_info *info)
{
  info->width = 640;
  info->height = 480;
  info->hf = 1.0f;
  info->dev_type = TIR5V3;
}

static void get_sn4_info(tir_info *info)
{
  info->width = 640;
  info->height = 480;
  info->hf = 2.0f;
  info->dev_type = SMARTNAV4;
}

static void get_sn3_info(tir_info *info)
{
  info->width = 400;
  info->height = 300;
  info->hf = 2.0f;
  info->dev_type = SMARTNAV3;
}

void ltr_int_get_tir_info(tir_info *info)
{
  assert(tir_iface != NULL);
  tir_iface->get_tir_info(info);
}

static void switch_green(bool state)
{
  if(state){
    turn_led_on_tir(TIR_LED_GREEN);
  }else{
    turn_led_off_tir(TIR_LED_GREEN);
  }
}

void switch_blue(bool state)
{
  if(state){
    turn_led_on_tir(TIR_LED_BLUE);
  }else{
    turn_led_off_tir(TIR_LED_BLUE);
  }
}

static void switch_red(bool state)
{
  if(state){
    turn_led_on_tir(TIR_LED_RED);
  }else{
    turn_led_off_tir(TIR_LED_RED);
  }
}

/*
static void switch_ir(bool state)
{
  if(state){
    turn_led_on_tir(TIR_LED_IR);
  }else{
    turn_led_off_tir(TIR_LED_IR);
  }
}
*/

char *ltr_int_find_firmware(dev_found dev)
{
  const char *fw_file;
  switch(dev){
    case TIR3:
    case TIR2:
    case SMARTNAV3:
    case TIR5V3:
      //no firmware needed
      fw_file = NULL;
      break;
    case TIR4:
      fw_file = "tir4.fw.gz";
      break;
    case TIR5:
      fw_file = "tir5.fw.gz";
      break;
    case TIR5V2:
      fw_file = "tir5v2.fw.gz";
      break;
    case SMARTNAV4:
      fw_file = "sn4.fw.gz";
      break;
    default:
      ltr_int_log_message("Unknown device!\n");
      return false;
      break;
  }
  char *fw_path;
  if(fw_file != NULL){
    fw_path = ltr_int_get_resource_path("tir_firmware", fw_file);
  }else{
    fw_path = NULL;
  }
  return fw_path;
}

static tir_interface tir2 = {
  .stop_camera_tir = stop_camera_tir2,
  .start_camera_tir = start_camera_tir2,
  .init_camera_tir = init_camera_tir2,
  .close_camera_tir = close_camera_tir2,
  .get_tir_info = get_tir2_info,
  .set_status_led_tir = set_status_led_tir4
};

static tir_interface tir3 = {
  .stop_camera_tir = stop_camera_tir3,
  .start_camera_tir = start_camera_tir3,
  .init_camera_tir = init_camera_tir3,
  .close_camera_tir = close_camera_tir3,
  .get_tir_info = get_tir3_info,
  .set_status_led_tir = set_status_led_tir4
};

static tir_interface tir4 = {
  .stop_camera_tir = stop_camera_tir4,
  .start_camera_tir = start_camera_tir4,
  .init_camera_tir = init_camera_tir4,
  .close_camera_tir = close_camera_tir4,
  .get_tir_info = get_tir4_info,
  .set_status_led_tir = set_status_led_tir4
};

static tir_interface tir5 = {
  .stop_camera_tir = stop_camera_tir5,
  .start_camera_tir = start_camera_tir5,
  .init_camera_tir = init_camera_tir5,
  .close_camera_tir = close_camera_tir5,
  .get_tir_info = get_tir5_info,
  .set_status_led_tir = set_status_led_tir5
};

static tir_interface smartnav4 = {
  .stop_camera_tir = stop_camera_sn4,
  .start_camera_tir = start_camera_sn4,
  .init_camera_tir = init_camera_sn4,
  .close_camera_tir = close_camera_sn4,
  .get_tir_info = get_sn4_info,
  .set_status_led_tir = set_status_led_sn4
};

static tir_interface smartnav3 = {
  .stop_camera_tir = stop_camera_sn3,
  .start_camera_tir = start_camera_sn3,
  .init_camera_tir = init_camera_sn3,
  .close_camera_tir = close_camera_tir4,
  .get_tir_info = get_sn3_info,
  .set_status_led_tir = set_status_led_sn3
};


static int rand_range(int l, int h)
{
  if(h <= l){
    return h;
  }
  int w = h - l + 1;
  if(w > 0){
    return l + (rand() % w);
  }else{
    return h;
  }
}

static bool send_packet_tir5v3(uint8_t packet[], int wait)
{
  // Expects packet of 24 bytes
  uint8_t r1 = (rand_range(1, 15) << 4) + rand_range(1,15);
  packet[0] ^= 0x69 ^ packet[r1 >> 4] ^ packet[r1 & 0x0F];
  packet[17] = r1;
  ltr_int_log_message("*tir5v3* Packet:"
                      " %02X %02X %02X %02X %02X %02X %02X %02X"
                      " %02X %02X %02X %02X %02X %02X %02X %02X"
                      " %02X %02X %02X %02X %02X %02X %02X %02X\n",
                      packet[0], packet[1], packet[2], packet[3], packet[4], packet[5],
                      packet[6], packet[7], packet[8], packet[9], packet[10], packet[11],
                      packet[12], packet[13], packet[14], packet[15], packet[16], packet[17],
                      packet[18], packet[19], packet[20], packet[21], packet[22], packet[23]
  );
  if(ltr_int_send_data(out_ep, packet, 24)){
    if(wait > 0){
      ltr_int_usleep(wait);
    }
    return true;
  }
  return false;
}



static bool send_packet_4_tir5v3(uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4, int wait)
{
  ltr_int_log_message("*tir5v3* Intent: 0x%02X%02X%02X%02X\n", v1, v2, v3, v4);
  uint8_t packet[24];
  unsigned int i;
  packet[0] = v1;
  for(i = 1; i < sizeof(packet); ++i){
    packet[i] = rand();
  }
  uint8_t r1 = rand_range(6, 14);
  packet[1] ^= (packet[1] ^ r1) & 8;
  packet[2] ^= (packet[2] ^ r1) & 4;
  packet[3] ^= (packet[3] ^ r1) & 2;
  packet[4] ^= (packet[4] ^ r1) & 1;

  packet[r1] = packet[16] ^ v2;
  packet[r1 - 1] = packet[19] ^ v3;
  packet[r1 + 1] = packet[18] ^ v4;

  return send_packet_tir5v3(packet, wait);
}

static bool send_packet_2_tir5v3(uint8_t v1, uint8_t v2, int wait)
{
  ltr_int_log_message("*tir5v3* Intent: 0x%02X%02X\n", v1, v2);
  uint8_t packet[24];
  unsigned int i;
  packet[0] = v1;
  for(i = 1; i < sizeof(packet); ++i){
    packet[i] = rand();
  }
  uint8_t r1 = rand_range(2, 14);
  uint8_t r2 = rand_range(0, 3);
  packet[1] ^= (packet[1] ^ r1) & 8;
  packet[2] ^= (packet[2] ^ r1) & 4;
  packet[3] ^= (packet[3] ^ r1) & 2;
  packet[4] ^= (packet[4] ^ r1) & 1;
  packet[r1] = ((v2 << 4) | (packet[r1] & 0x0F)) ^ (packet[16] & 0xF0);
  packet[r1 + 1] = (r2 << 6) | (packet[r1 + 1] & 0x3F);
  return send_packet_tir5v3(packet, wait);
}

static bool send_packet_1_tir5v3(uint8_t v1, int wait)
{
  ltr_int_log_message("*tir5v3* Intent: 0x%02X\n", v1);
  uint8_t packet[24];
  unsigned int i;
  packet[0] = v1;
  for(i = 1; i < sizeof(packet); ++i){
    packet[i] = rand();
  }
  return send_packet_tir5v3(packet, wait);
}

static bool set_threshold_tir5v3(unsigned int t)
{
  return send_packet_4_tir5v3(0x19, 0x05, t >> 7, (t << 1) & 0xFF, 50000);
}

static bool set_ir_led_tir5v3(bool ir)
{
  return send_packet_4_tir5v3(0x19, 0x09, 00, ir ? 0x01 : 0x00, 50000);
}

static bool set_ir_brightness_raw_tir5v3(unsigned int b)
{
  bool res = true;
  res &= send_packet_4_tir5v3(0x23, 0x35, 0x02, b & 0xFF, 50000);
  res &= send_packet_4_tir5v3(0x23, 0x35, 0x01, b >> 8, 50000);
  res &= send_packet_4_tir5v3(0x23, 0x35, 0x00, 0x00, 50000);
  res &= send_packet_4_tir5v3(0x23, 0x3B, 0x8F, (b >> 4) & 0xFF, 50000);
  res &= send_packet_4_tir5v3(0x23, 0x3B, 0x8E, b >> 12, 50000);
  return res;
}

static bool set_ir_brightness_tir5v3(unsigned int b)
{
  unsigned int value_map[] = {150, 250, 350};
  b -= 5;
  if(b < 3){
    return set_ir_brightness_raw_tir5v3(value_map[b] << 4);
  }else{
    return false;
  }
}

static bool set_status_led_tir5v3(bool running)
{
  uint8_t brightness = ltr_int_tir_get_status_brightness();
  uint8_t leds = running ? 0x22 : 0x33;
  return send_packet_4_tir5v3(0x19, 0x04, brightness, leds, 50000);
}

static bool read_status_tir5v3(tir_status_t *status)
{
  assert(status != NULL);
  size_t t;
  int counter = 0;
  struct timeval tv1, tv2;
  struct timezone tz;
  gettimeofday(&tv1, &tz);
  memset(status, 0, sizeof(tir_status_t));
  while(counter < 20){
    ltr_int_log_message("Requesting status.\n");
    if(!send_packet_2_tir5v3(0x1A, 0x07, 50000)){
      ltr_int_log_message("Couldn't send status request!\n");
      return false;
    }

    while(1){
      if(!ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 500)){
        ltr_int_log_message("Couldn't receive status!\n");
        return false;
      }
      if((t > 2) && (ltr_int_packet[0] >= 0x07) && (ltr_int_packet[1] == 0x20)){
        break;
      }
      gettimeofday(&tv2, &tz);
      if(time_diff_msec(&tv2, &tv1) > 100){
        ltr_int_log_message("Status request timed out, will try again...\n");
        tv1 = tv2;
        break;
      }
    }
    if((t > 2) && (ltr_int_packet[0] >= 0x07) && (ltr_int_packet[1] == 0x20)){
      break;
    }
    counter++;
  }
  ltr_int_log_message("Status packet: %02X %02X %02X %02X %02X %02X %02X\n",
    ltr_int_packet[0], ltr_int_packet[1], ltr_int_packet[2], ltr_int_packet[3],
    ltr_int_packet[4], ltr_int_packet[5], ltr_int_packet[6]);

  status->fw_loaded = (ltr_int_packet[3] == 1) ? true : false;
  status->cfg_flag = 0;
  status->fw_cksum = 0;
  return true;
}

static bool read_rom_data_tir5v3()
{
  size_t t;
  ltr_int_log_message("Flushing packets...\n");
  do{
    ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  }while(t > 0);
  ltr_int_log_message("Sending get_conf request.\n");
  int counter = 0;
  struct timeval tv1, tv2;
  struct timezone tz;
  gettimeofday(&tv1, &tz);
  while(counter < 10){
    if(!send_packet_1_tir5v3(*Get_conf, 50000)){
      ltr_int_log_message("Couldn't send config data request!\n");
      return false;
    }

    while(1){
      ltr_int_log_message("Requesting data...\n");
      if(!ltr_int_receive_data(cfg_in_ep, ltr_int_packet, sizeof(ltr_int_packet), &t, 500)){
        ltr_int_log_message("Couldn't receive status!\n");
        return false;
      }
      //Tir4/5 0x14, Tir3 0x09, SN4 0x15...
      if((t > 2) && (ltr_int_packet[1] == 0x40)){
        break;
      }
      gettimeofday(&tv2, &tz);
      if(time_diff_msec(&tv2, &tv1) > 100){
        ltr_int_log_message("Request for data timed out. Will try later...\n");
        tv1 = tv2;
        break;
      }
    }
    //Tir4/5 0x14, Tir3 0x09, SN4 0x15...
    if((t > 2) && (ltr_int_packet[1] == 0x40)){
      break;
    }
    counter++;
  }
  return true;
}

static bool start_camera_tir5v3()
{
  set_ir_led_tir5v3(ir_on);
  set_ir_brightness_tir5v3(ltr_int_tir_get_ir_brightness());
  set_threshold_tir5v3(ltr_int_tir_get_threshold());
  set_threshold_tir5v3(ltr_int_tir_get_threshold());
  send_packet_4_tir5v3(0x19, 0x04, 0x00, 0x00, 50000);
  send_packet_4_tir5v3(0x19, 0x03, 0x00, 0x05, 50000);
  send_packet_2_tir5v3(0x1A, 0x04, 50000);
  set_ir_led_tir5v3(ir_on);
  if(ltr_int_tir_get_status_indication()){
    set_status_led_tir5v3(true);
  }
  return true;
}

static bool stop_camera_tir5v3()
{
  send_packet_2_tir5v3(0x1A, 0x05, 50000);
  set_ir_led_tir5v3(false);
  send_packet_2_tir5v3(0x1A, 0x06, 50000);
  send_packet_1_tir5v3(0x13, 50000);
  return true;
}
static bool init_camera_tir5v3(bool force_fw_load, bool p_ir_on)
{
  (void) force_fw_load;
  ir_on = p_ir_on;
  tir_status_t status;

  send_packet_2_tir5v3(0x1A, 0x00, 50000);
  send_packet_2_tir5v3(0x1A, 0x00, 50000);
  send_packet_1_tir5v3(0x13, 50000);
  read_status_tir5v3(&status);
  send_packet_2_tir5v3(0x1A, 0x01, 50000);
  read_status_tir5v3(&status);
  send_packet_2_tir5v3(0x1A, 0x02, 50000);
  read_status_tir5v3(&status);

  int cntr = 3;
  while(cntr){
    send_packet_2_tir5v3(0x1A, 0x03, 50000);
    read_status_tir5v3(&status);
    if(status.fw_loaded){
      break;
    }
    --cntr;
  }
  read_rom_data_tir5v3();
  set_threshold_tir5v3(0x96);
  set_ir_brightness_raw_tir5v3(0x780);
  set_ir_led_tir5v3(true);

  send_packet_4_tir5v3(0x19, 0x03, 0x00, 0x05, 50000);
  send_packet_4_tir5v3(0x19, 0x04, 0x00, 0x00, 50000);

  return true;
}

static bool close_camera_tir5v3()
{
  stop_camera_tir5v3();
  ltr_int_log_message("Closing the TIR5 camera.\n");
  set_threshold_tir5v3(0x96);
  set_ir_brightness_raw_tir5v3(0x780);
  set_ir_led_tir5v3(true);

  send_packet_4_tir5v3(0x19, 0x03, 0x00, 0x05, 50000);
  set_ir_led_tir5v3(false);
  send_packet_2_tir5v3(0x1A, 0x06, 50000);
  send_packet_1_tir5v3(0x13, 50000);
  send_packet_2_tir5v3(0x1A, 0x00, 50000);
  ltr_int_finish_usb(TIR_INTERFACE);
  ltr_int_log_message("TIR5 camera closed.\n");
  return true;
}

static tir_interface tir5v3 = {
  .stop_camera_tir = stop_camera_tir5v3,      //
  .start_camera_tir = start_camera_tir5v3,    //
  .init_camera_tir = init_camera_tir5v3,      //
  .close_camera_tir = close_camera_tir5v3,    //
  .get_tir_info = get_tir5v3_info,            //x
  .set_status_led_tir = set_status_led_tir5v3 //x
};

