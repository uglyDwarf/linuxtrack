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
unsigned char Set_threshold[] = {0x15, 0x96, 0x01, 0x00};
unsigned char Set_exposure_h[] =  {0x23, 0x42, 0x08, 0x01, 0x00, 0x00};
unsigned char Set_exposure_l[] =  {0x23, 0x42, 0x10, 0x8F, 0x00, 0x00};
unsigned char Set_ir_brightness[] =  {0x10, 0x00, 0x02, 0x00, 0xA0};


unsigned char unk_2[] =  {0x12, 0x01};
unsigned char unk_3[] =  {0x13, 0x01};
unsigned char unk_8[] =  {0x15, 0x96, 0x01, 0x00};
unsigned char unk_1[] =  {0x17, 0x01};
unsigned char unk_9[] =  {0x19, 0x04, 0x10, 0x00, 0x00};
unsigned char unk_13[] = {0x19, 0x04, 0x10, 0x03, 0x00};
unsigned char unk_7[] =  {0x19, 0x05, 0x10, 0x10, 0x00};
unsigned char unk_4[] =  {0x19, 0x09, 0x10, 0x05, 0x01};
unsigned char unk_11[] = {0x19, 0x09, 0x10, 0x07, 0x01};

static bool ir_on = false;

dev_found device = NONE;

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


unsigned char packet[4096];

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


static bool read_status_tir(tir_status_t *status)
{
  assert(status != NULL);
  size_t t;
  if(!send_data(Get_status, sizeof(Get_status))){
    log_message("Couldn't send status request!\n");
    return false;
  }
  int counter = 0;
  while(counter < 10){
    if(!receive_data(packet, sizeof(packet), &t, 2000)){
      log_message("Couldn't receive status!\n");
      return false;
    }
    if((packet[0] == 0x07) && (packet[1] == 0x20)){
      break;
    }
    usleep(10000);
    counter++;
  }
  log_message("Status packet: %02X %02X %02X %02X %02X %02X %02X\n",
    packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], 
    packet[6]);
  
  status->fw_loaded = (packet[3] == 1) ? true : false;
  status->cfg_flag = packet[6];
  status->fw_cksum = (((unsigned int)packet[4]) << 8) + (unsigned int)packet[5]; 
  return true;
}

static bool read_rom_data_tir()
{
  size_t t;
  if(!send_data(Get_conf, sizeof(Get_conf))){
    log_message("Couldn't send config data request!\n");
    return false;
  }
  int counter = 0;
  while(counter < 10){
    if(!receive_data(packet, sizeof(packet), &t, 2000)){
      log_message("Couldn't receive status!\n");
      return false;
    }
    if((packet[0] == 0x14) && (packet[1] == 0x40)){
      break;
    }
    usleep(10000);
    counter++;
  }
  return true;
}



static bool load_firmware(firmware_t *fw, char data_path[])
{
  assert(fw != NULL);
  assert(data_path != NULL);
  gzFile *f;
  unsigned int tsize = FW_SIZE_INCREMENT;
  unsigned char *tbuf = my_malloc(tsize);
  unsigned char *ptr = tbuf;
  unsigned int read = 0;
  size_t *size = &(fw->size);
  *size = 0;
  
  char *fw_path;
  switch(device){
    case TIR4:
      fw_path = my_strcat(data_path, "/tir4.fw.gz");
      break;
    case TIR5:
      fw_path = my_strcat(data_path, "/tir5.fw.gz");
      break;
    default:
      log_message("Unknown device!\n");
      return false;
      break;
  }
  log_message("Loading firmware '%s'\n", fw_path);
  f = gzopen(fw_path, "rb");
  if(f == NULL){
    log_message("Couldn't open firmware (%s)!\n", fw_path);
    free(fw_path);
    return false;
  }
  free(fw_path);
  
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
    fw->firmware = tbuf;
    cksum_firmware(fw);
    log_message("Size: %d  Cksum: %04X\n", fw->size, fw->cksum);
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

bool flush_fifo_tir()
{
  if(device == TIR5){
    usleep(100000);
  }
  return send_data(Fifo_flush,sizeof(Fifo_flush));
}


bool stop_camera_tir()
{
  send_data(Video_off,sizeof(Video_off));
  flush_fifo_tir();
  turn_led_off_tir(TIR_LED_IR);
  turn_led_off_tir(TIR_LED_RED);
  turn_led_off_tir(TIR_LED_GREEN);
  turn_led_off_tir(TIR_LED_BLUE);
  send_data(Camera_stop,sizeof(Camera_stop));
  usleep(50000);
  return true;
}

bool start_camera_tir()
{
  stop_camera_tir();
  send_data(Video_on,sizeof(Video_on));
  if(ir_on){ 
    turn_led_on_tir(TIR_LED_IR);
  }
  if(device == TIR4){
    flush_fifo_tir();
  }
  return true;
}

bool init_camera_tir(char data_path[], bool force_fw_load, bool p_ir_on)
{
  tir_status_t status;
  size_t t;
  
  ir_on = p_ir_on;

  if(!stop_camera_tir()){
    return false;
  }
  //To flush any pending packets...
  receive_data(packet, sizeof(packet), &t, 100);
  receive_data(packet, sizeof(packet), &t, 100);
  receive_data(packet, sizeof(packet), &t, 100);
  if(!read_rom_data_tir()){
    return false;
  }
  if(!read_status_tir(&status)){
    return false;
  }
  firmware_t firmware;
  if(!load_firmware(&firmware, data_path)){
    log_message("Error loading firmware!\n");
    return false;
  }
  
  if(force_fw_load | (!status.fw_loaded) | (status.fw_cksum != firmware.cksum)){
    upload_firmware(&firmware);
  }

  if(!read_status_tir(&status)){
    return false;
  }
  
  if(status.fw_cksum != firmware.cksum){
    log_message("Firmware not loaded correctly!");
    return false;
  }
  
  if(status.cfg_flag == 1){
    send_data(Cfg_reload,sizeof(Cfg_reload));
    while(status.cfg_flag != 2){
      if(!read_status_tir(&status)){
	return false;
      }
    }
    if(device == TIR5){
      send_data(Set_ir_brightness,sizeof(Set_ir_brightness));
      send_data(Set_exposure_h,sizeof(Set_exposure_h));
      send_data(Set_exposure_l,sizeof(Set_exposure_l));
      send_data(Set_threshold,sizeof(Set_threshold));
    }
  }else if(status.cfg_flag != 2){
    log_message("TIR configuration problem!\n");
    return false;
  }
  
  
  if(device == TIR5){
    send_data(Precision_mode,sizeof(Precision_mode));
  }
  
  free(firmware.firmware);
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

  if(!init_camera_tir(data_path, force_fw_load, ir_on))
    return false;
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

//bool read_frame(plist *blob_list)
//{
//  return read_blobs_tir(blob_list, );
//}

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
      *h = 2 * 288;
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
