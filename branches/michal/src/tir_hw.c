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
#include "pref_int.h"
#include "tir.h"

#define TIR_CONFIGURATION 1
#define TIR_INTERFACE 0
#define TIR_IN_EP  0x01
#define TIR_OUT_EP 0x82

#define FW_SIZE_INCREMENT 50000

//Known packets
unsigned char Video_off[] = {0x14, 0x01};
unsigned char Video_on[] = {0x14, 0x00};
unsigned char Fifo_flush[] = {0x12};
unsigned char Camera_stop[] = {0x13};
unsigned char Fpga_init[] = {0x1b};
unsigned char Cfg_reload[] = {0x20};
unsigned char Get_status[] = {0x1d};
unsigned char Get_conf[] = {0x17};
unsigned char Precision_mode[] = {0x19, 0x03, 0x10, 0x00, 0x05};
unsigned char Set_ir_brightness[] =  {0x10, 0x00, 0x02, 0x00, 0xA0};

//Unknown packets
unsigned char unk_2[] =  {0x12, 0x01};
unsigned char unk_3[] =  {0x13, 0x01};
unsigned char unk_1[] =  {0x17, 0x01};
unsigned char unk_7[] =  {0x19, 0x05, 0x10, 0x10, 0x00};

static bool ir_on = true;
static unsigned char status_brightness = 3; //0 - highest, 3 lowest
static unsigned char ir_brightness = 5; //5 - lowest, 7 - highest

dev_found device = NONE;

tir_interface tir4;
tir_interface tir5;
tir_interface *tir_iface = NULL;

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
extern bool threshold_changed;
extern bool status_brightness_changed;
extern bool ir_led_brightness_changed;
extern bool signal_flag;
extern pref_id threshold;
extern pref_id stat_bright;
extern pref_id ir_bright;

void switch_red(bool state);
void switch_green(bool state);


void set_status_brightness_tir(unsigned char b)
{
  if(b > 3){
    b = 3;
  }
  status_brightness = b;
}

void set_ir_brightness_tir(unsigned char b)
{
  if(b < 5){
    b = 5;
  }
  if(b > 7){
    b = 7;
  }
  ir_brightness = b;
}



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



static bool load_firmware(firmware_t *fw, const char data_path[])
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

bool set_threshold(unsigned int val)
{
  unsigned char pkt[] = {0x15, 0x96, 0x01, 0x00};
  if(val < 30){
    val = 30;
  }
  if(val > 253){
    val = 253;
  }
  pkt[1] = val;
  return send_data(pkt, sizeof(pkt));
}

static bool set_status_led_tir5(bool running)
{
  unsigned char pkt[] =  {0x19, 0x04, 0x10, 0x00, 0x00};
  if(status_brightness_changed){
    status_brightness_changed = false;
    int new_brightness = get_int(stat_bright); 
    if(new_brightness != 0){
      set_status_brightness_tir(new_brightness);
    }
  }
  pkt[3] = status_brightness;
  if(running){
    pkt[4] = 20;
  }else{
    pkt[4] = 33;
  }
  return send_data(pkt, sizeof(pkt));
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

bool set_status_led_tir(bool running)
{
  assert(tir_iface != NULL);
  if(signal_flag){
    return tir_iface->set_status_led_tir(running);
  }
  return true;
}

static bool control_status_led_tir(bool status1, bool status2)
{
  unsigned char pkt[] =  {0x19, 0x04, 0x10, 0x00, 0x00};
  pkt[3] = status_brightness;
  pkt[4] = (status1 ? 0x20 : 0) | (status2 ? 0x02 : 0);
  return send_data(pkt, sizeof(pkt));
}

static bool control_ir_led_tir(bool ir)
{
  unsigned char pkt[] =  {0x19, 0x09, 0x10, 0x00, 0x00};
  if(ir_led_brightness_changed){
    ir_led_brightness_changed = false;
    int new_brightness = get_int(ir_bright); 
    if(new_brightness != 0){
      set_ir_brightness_tir(new_brightness);
    }
  }
  pkt[3] = ir ? ir_brightness : 0;
  pkt[4] = ir ? 0x01 : 0;
  return send_data(pkt, sizeof(pkt));
}

static bool set_exposure(unsigned int exp)
{
  unsigned char Set_exposure_h[] =  {0x23, 0x42, 0x08, 0x01, 0x00, 0x00};
  unsigned char Set_exposure_l[] =  {0x23, 0x42, 0x10, 0x8F, 0x00, 0x00};
  
  Set_exposure_h[3] = exp >> 8;
  Set_exposure_l[3] = exp & 0xFF;
  return send_data(Set_exposure_h, sizeof(Set_exposure_h)) && 
    send_data(Set_exposure_l, sizeof(Set_exposure_l));
  
}

bool stop_camera_tir4()
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

bool stop_camera_tir5()
{
  send_data(Video_off,sizeof(Video_off));
  usleep(50000);
  send_data(Video_off,sizeof(Video_off));
  usleep(50000);
  send_data(Video_off,sizeof(Video_off));
  usleep(50000);
  send_data(Video_off,sizeof(Video_off));
  usleep(50000);
  send_data(Video_off,sizeof(Video_off));
  usleep(50000);
  send_data(unk_2,sizeof(unk_2));
  send_data(unk_3,sizeof(unk_3));
  send_data(Video_off,sizeof(Video_off));
  usleep(50000);
  send_data(Video_off,sizeof(Video_off));
  usleep(50000);
  send_data(Video_off,sizeof(Video_off));
  usleep(50000);
  send_data(Video_off,sizeof(Video_off));
  usleep(50000);
  send_data(Video_off,sizeof(Video_off));
  usleep(50000);
  flush_fifo_tir();
  send_data(Camera_stop,sizeof(Camera_stop));
  control_ir_led_tir(false);
  return true;
}

bool stop_camera_tir()
{
  assert(tir_iface != NULL);
  return tir_iface->stop_camera_tir();
}

bool start_camera_tir4()
{
  stop_camera_tir();
  send_data(Video_on,sizeof(Video_on));
  if(ir_on){ 
    turn_led_on_tir(TIR_LED_IR);
  }
  flush_fifo_tir();
  if(signal_flag) 
    set_status_led_tir4(true);
  return true;
}

bool start_camera_tir5()
{
  stop_camera_tir();
  send_data(Video_on,sizeof(Video_on));
  if(ir_on){ 
    control_ir_led_tir(true);
  }else{
    control_ir_led_tir(false);
  }
  if(signal_flag) 
    set_status_led_tir5(true);
  return true;
}

bool start_camera_tir()
{
  assert(tir_iface != NULL);
  return tir_iface->start_camera_tir();
}


bool init_camera_tir4(const char data_path[], bool force_fw_load, bool p_ir_on)
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
      set_exposure(0x18F);
    }
    set_threshold(0x82);
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



bool init_camera_tir5(const char data_path[], bool force_fw_load, bool p_ir_on)
{
  tir_status_t status;
  ir_on = p_ir_on;
  
  stop_camera_tir();
  
  read_rom_data_tir();
  control_ir_led_tir(true);
  flush_fifo_tir();
  set_exposure(0x18F);
  read_status_tir(&status);
  firmware_t firmware;
  if(!load_firmware(&firmware, data_path)){
    log_message("Error loading firmware!\n");
    return false;
  }
  upload_firmware(&firmware);
  free(firmware.firmware);
  send_data(unk_7,sizeof(unk_7));
  flush_fifo_tir();
  send_data(Camera_stop,sizeof(Camera_stop));
  read_status_tir(&status);
  send_data(Cfg_reload,sizeof(Cfg_reload));
  set_threshold(0x96);
  set_exposure(0x18F);
  control_ir_led_tir(true);
  control_status_led_tir(false, false);
  control_status_led_tir(false, false);
  send_data(Precision_mode,sizeof(Precision_mode));
  control_status_led_tir(false, false);
  control_ir_led_tir(true);
  set_threshold(0x76);
  set_exposure(0x15E);
  control_status_led_tir(false, false);
  send_data(Precision_mode,sizeof(Precision_mode));
  flush_fifo_tir();
  send_data(Camera_stop,sizeof(Camera_stop));
  control_ir_led_tir(true);
  send_data(Video_on,sizeof(Video_on));
  return true;
}

bool init_camera_tir(const char data_path[], bool force_fw_load, bool p_ir_on)
{
  assert(tir_iface != NULL);
  return tir_iface->init_camera_tir(data_path, force_fw_load, p_ir_on);
}

bool open_tir(const char data_path[], bool force_fw_load, bool ir_on)
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
  
  switch(device){
    case TIR4:
      tir_iface = &tir4;
      break;
    case TIR5:
      tir_iface = &tir5;
      break;
    default:
      log_message("No device!\n");
      return false;
      break;
  }
  if(!init_camera_tir(data_path, force_fw_load, ir_on)){
    return false;
  }
  start_camera_tir();
  return true;
}

bool pause_tir()
{
  bool res = stop_camera_tir();
  set_status_led_tir(false);
  return res;
}

bool resume_tir()
{
  bool res = start_camera_tir();
  set_status_led_tir(true);
  return res;
}

bool close_camera_tir4(){
  stop_camera_tir();
  finish_usb(TIR_INTERFACE);
  return true;
}

bool close_camera_tir5()
{
  stop_camera_tir();
  set_threshold(0x96);
  set_exposure(0x18F);
  control_ir_led_tir(true);
  control_status_led_tir(false, false);
  control_status_led_tir(false, false);
  send_data(Precision_mode,sizeof(Precision_mode));
  turn_led_off_tir(TIR_LED_IR);
  if(!send_data(Fpga_init, sizeof(Fpga_init))){
    log_message("Couldn't init fpga!\n");
  }
  finish_usb(TIR_INTERFACE);
  return true;
}

bool close_tir()
{
  assert(tir_iface != NULL);
  return tir_iface->close_camera_tir();
}



void get_res_tir4(unsigned int *w, unsigned int *h, float *hf)
{
  *w = 710;
  *h = 2 * 288;
  *hf = 2.0f;
}

void get_res_tir5(unsigned int *w, unsigned int *h, float *hf)
{
  *w = 640;
  *h = 480;
  *hf = 1.0f;
}

void get_res_tir(unsigned int *w, unsigned int *h, float *hf)
{
  assert(tir_iface != NULL);
  tir_iface->get_res_tir(w, h, hf);
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

tir_interface tir4 = {
  .stop_camera_tir = stop_camera_tir4,
  .start_camera_tir = start_camera_tir4,
  .init_camera_tir = init_camera_tir4,
  .close_camera_tir = close_camera_tir4,
  .get_res_tir = get_res_tir4,
  .set_status_led_tir = set_status_led_tir4
};

tir_interface tir5 = {
  .stop_camera_tir = stop_camera_tir5,
  .start_camera_tir = start_camera_tir5,
  .init_camera_tir = init_camera_tir5,
  .close_camera_tir = close_camera_tir5,
  .get_res_tir = get_res_tir5,
  .set_status_led_tir = set_status_led_tir5
};


