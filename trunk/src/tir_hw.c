#include <stdbool.h>
#include <stdio.h>
#include <zlib.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "tir_hw.h"
#include "tir_img.h"
#include "usb_ifc.h"
#include "utils.h"

#define TIR_CONFIGURATION 1
#define TIR_INTERFACE 0
#define TIR_IN_EP  0x01
#define TIR_OUT_EP 0x82

#define FW_SIZE_INCREMENT 50000

unsigned char Video_off[] = {0x14, 0x01};
unsigned char Video_on[] = {0x14, 0x00};
unsigned char Fifo_flush[] = {0x12};
unsigned char Camera_stop[] = {0x13};
unsigned char Fpga_init[] = {0x1b};
unsigned char Cfg_reload[] = {0x20};
unsigned char Get_status[] = {0x1d};
unsigned char Get_conf[] = {0x17};
unsigned char Precision_mode[] = {0x19, 0x03, 0x10, 0x00, 0x05};
unsigned char Set_threshold[] = {0x15, 0xFD, 0x01, 0x00};

unsigned char unk_2[] =  {0x12, 0x01};
unsigned char unk_3[] =  {0x13, 0x01};
unsigned char unk_8[] =  {0x15, 0x96, 0x01, 0x00};
unsigned char unk_1[] =  {0x17, 0x01};
unsigned char unk_9[] =  {0x19, 0x04, 0x10, 0x00, 0x00};
unsigned char unk_13[] = {0x19, 0x04, 0x10, 0x03, 0x00};
unsigned char unk_7[] =  {0x19, 0x05, 0x10, 0x10, 0x00};
unsigned char unk_4[] =  {0x19, 0x09, 0x10, 0x05, 0x01};
unsigned char unk_11[] = {0x19, 0x09, 0x10, 0x07, 0x01};
unsigned char unk_5[] =  {0x23, 0x42, 0x08, 0x01, 0x00, 0x00};
unsigned char unk_6[] =  {0x23, 0x42, 0x10, 0x8F, 0x00, 0x00};

static bool ir_on = false;

dev_found device = NONE;

unsigned char packet[4096];

static unsigned int cksum_firmware(unsigned char *firmware, int size)
{
  unsigned int cksum = 0;
  unsigned int byte = 0;
  
  while(size > 0){
    byte = (unsigned int)*firmware;

    cksum += byte;
    byte = byte << 4;
    cksum ^= byte;

    --size;
    ++firmware; 
  }
  return cksum & 0xffff;
}


static bool load_firmware(char *fname, unsigned char *buffer[], unsigned int *size)
{
  gzFile *f;
  unsigned int tsize = FW_SIZE_INCREMENT;
  unsigned char *tbuf = malloc(tsize);
  unsigned char *ptr = tbuf;
  unsigned int read = 0;
  *size = 0;
  
  if(tbuf == NULL){
    return false;
  }

  f = gzopen(fname, "rb");
  if(f == NULL){
    log_message("Couldn't open firmware (%s)!\n", fname);
    return false;
  }
  
  while(1){
    read = gzread(f, ptr, FW_SIZE_INCREMENT);
    *size += read;
    if(read < FW_SIZE_INCREMENT){
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
    *buffer = tbuf;
    return true;
  }else{
    return false;
  }
}

static bool check_firmware(unsigned char *firmware, unsigned int size)
{
  unsigned int cksum;
  unsigned int cksum_uploaded;
  size_t t;
  
  while(receive_data(packet, sizeof(packet), &t));


  if(!send_data(Get_status, sizeof(Get_status))){
    log_message("Couldn't send data!\n");
    return false;
  }
  if(!receive_data(packet, sizeof(packet), &t)){
    log_message("Couldn't receive data!\n");
    return false;
  }
  cksum = cksum_firmware(firmware, size);
  cksum_uploaded = ((unsigned int)packet[4] << 8) | (unsigned int)packet[5];
  if(cksum != cksum_uploaded){
    log_message("Cksum doesn't match! Uploaded: %04X, Computed: %04X\n",
      cksum_uploaded, cksum);
    log_message("Status packet: %02X %02X %02X %02X %02X %02X %02X %02X\n",
      packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], 
      packet[6]);
    return false;
  }
  return true;
}

static bool upload_firmware(unsigned char *firmware, unsigned int size)
{
  unsigned char buf[62];
  unsigned char len;
  
  if(!send_data(Fpga_init, sizeof(Fpga_init))){
    log_message("Couldn't init fpga!\n");
    return false;
  }

  buf[0] = 0x1c;
  while(size>0){
    len = (size > 60) ? 60 : size;
    size -= len;
    buf[1] = len;
    memcpy(&(buf[2]), firmware, len);
    firmware += len;

    if(!send_data(buf, len + 2)){
      log_message("Couldn't send data!\n");
      return false;
    }
  }
  log_message("Firmware uploaded!\n");
  
  return true;
}

static bool wiggle_leds_tir(unsigned char leds, unsigned char mask)
{
  unsigned char msg[3] = {0x10, leds, mask};
  if(!send_data(msg, sizeof(msg))){
    log_message("Problem wiggling LEDs\n");
    return false;
  }
  return true;
}

bool turn_led_off_tir(unsigned char leds)
{
  leds &= 0xf0;
  return wiggle_leds_tir(0, leds);
}

bool turn_led_on_tir(unsigned char leds)
{
  leds &= 0xf0;
  return wiggle_leds_tir(leds, leds);
}


bool stop_camera_tir()
{
  send_data(Video_off,sizeof(Video_off));
  if(device == TIR5){
      send_data(unk_2,sizeof(unk_2));
      send_data(unk_3,sizeof(unk_3));
  }
  
  send_data(Fifo_flush,sizeof(Fifo_flush));
  send_data(Camera_stop,sizeof(Camera_stop));
  turn_led_off_tir(TIR_LED_IR);
  if(device == TIR5){
    send_data(unk_8,sizeof(unk_8));
    send_data(unk_5,sizeof(unk_5));
    send_data(unk_6,sizeof(unk_6));
    send_data(unk_4,sizeof(unk_4));
    send_data(unk_13,sizeof(unk_13));
    send_data(unk_9,sizeof(unk_9));
    send_data(Precision_mode,sizeof(Precision_mode));
  }
  
  return true;
}

bool start_camera_tir()
{
  stop_camera_tir();
  send_data(Get_conf,sizeof(Get_conf));
  send_data(Get_status,sizeof(Get_status));
  send_data(Video_on,sizeof(Video_on));
  if(ir_on){ 
    turn_led_on_tir(TIR_LED_IR);
  }
  return true;
}

bool init_camera_tir(char data_path[], bool force_fw_load, bool p_ir_on)
{
  unsigned char *fw = NULL;
  unsigned int fw_size = 0;
  
  ir_on = p_ir_on;

  send_data(Video_off,sizeof(Video_off));
  if(device == TIR5){
      send_data(unk_2,sizeof(unk_2));
      send_data(unk_3,sizeof(unk_3));
  }
  send_data(Video_off,sizeof(Video_off));
  send_data(Fifo_flush,sizeof(Fifo_flush));
  
  stop_camera_tir();
  
  if(device == TIR5){
      send_data(Get_conf,sizeof(Get_conf));
      send_data(unk_4,sizeof(unk_4));
      send_data(Fifo_flush,sizeof(Fifo_flush));
      send_data(unk_5,sizeof(unk_5));
  }
  
  send_data(Fifo_flush,sizeof(Fifo_flush));
  send_data(Camera_stop,sizeof(Camera_stop));
  
  char *fw_path;
  switch(device){
    case TIR4:
      fw_path = my_strcat(data_path, "/tir4.fw.gz");
      load_firmware(fw_path, &fw, &fw_size);
      free(fw_path);
      break;
    case TIR5:
      fw_path = my_strcat(data_path, "/tir5.fw.gz");
      load_firmware(fw_path, &fw, &fw_size);
      free(fw_path);
      break;
    default:
      log_message("Unknown device!\n");
      return false;
      break;
  }
  if(force_fw_load || (check_firmware(fw, fw_size) == false)){
    upload_firmware(fw, fw_size);
    if(check_firmware(fw, fw_size) == false){
      log_message("Failed to upload firmware!\n");
      return false;
    }
  }else{
    log_message("Not loading firmware - it is already loaded!\n");
  }
  
  if(device == TIR5){
    send_data(unk_7,sizeof(unk_7));
  }
  
  if(ir_on){
    turn_led_on_tir(TIR_LED_IR);
  }
  
  send_data(Fifo_flush,sizeof(Fifo_flush));
  send_data(Camera_stop,sizeof(Camera_stop));
  send_data(Cfg_reload,sizeof(Cfg_reload));
  if(device == TIR5){
    send_data(unk_8,sizeof(unk_8));
    send_data(unk_5,sizeof(unk_5));
    send_data(unk_6,sizeof(unk_6));
    send_data(unk_4,sizeof(unk_4));
    send_data(unk_9,sizeof(unk_9));
    send_data(Precision_mode,sizeof(Precision_mode));
    send_data(unk_9,sizeof(unk_9));
    send_data(unk_11,sizeof(unk_11));
    //send_data(Set_threshold,sizeof(Set_threshold));
    send_data(unk_13,sizeof(unk_13));
    send_data(Precision_mode,sizeof(Precision_mode));
    send_data(Fifo_flush,sizeof(Fifo_flush));
    send_data(Camera_stop,sizeof(Camera_stop));
    send_data(unk_11,sizeof(unk_11));
    send_data(Video_on,sizeof(Video_on));  
  }
  
  if(device == TIR4){
    send_data(Video_off,sizeof(Video_off));
    send_data(Fifo_flush,sizeof(Fifo_flush));
    send_data(Video_on,sizeof(Video_on));
  }
  free(fw);
  return true;
}

bool open_tir(char data_path[], bool force_fw_load, bool ir_on)
{
  if(!init_usb()){
    log_message("Init failed!\n");
    return false;
  }
  if((device = find_tir()) == NONE){
    log_message("Tir not found!\n");
    return false;
  }
  
  if(!prepare_device(TIR_CONFIGURATION, TIR_INTERFACE, TIR_OUT_EP, TIR_IN_EP)){
    log_message("Couldn't prepare!\n");
    return false;
  }

  init_camera_tir(data_path, force_fw_load, ir_on);
  start_camera_tir();
  return true;
}

bool pause_tir()
{
  return stop_camera_tir();
}

bool resume_tir()
{
  return start_camera_tir();
}

bool read_frame(plist *blob_list)
{
  return read_blobs_tir(blob_list);
}

bool close_tir()
{
  stop_camera_tir();
  turn_led_off_tir(TIR_LED_RED);
  turn_led_off_tir(TIR_LED_GREEN);
  turn_led_off_tir(TIR_LED_BLUE);
  turn_led_off_tir(TIR_LED_IR);
  finish_usb(TIR_INTERFACE);
  return true;
}

void get_res_tir(unsigned int *w, unsigned int *h)
{
  switch(device){
    case TIR4:
      *w = 710;
      *h = 288;
      break;
    case TIR5:
      *w = 640;
      *h = 480;
      break;
    default:
      assert(0);
      break;
  }
  
}

void switch_green(bool state)
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

void switch_red(bool state)
{
  if(state){
    turn_led_on_tir(TIR_LED_RED);
  }else{
    turn_led_off_tir(TIR_LED_RED);
  }
}

void switch_ir(bool state)
{
  if(state){
    turn_led_on_tir(TIR_LED_IR);
  }else{
    turn_led_off_tir(TIR_LED_IR);
  }
}
