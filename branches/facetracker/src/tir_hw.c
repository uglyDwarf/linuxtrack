#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <zlib.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "tir_hw.h"
#include "tir_img.h"
#include "usb_ifc.h"
#include "utils.h"
#include "tir_driver_prefs.h"
#include "tir.h"

#define TIR_CONFIGURATION 1
#define TIR_INTERFACE 0
#define TIR_IN_EP  0x01
#define TIR3_IN_EP  0x02
#define TIR_OUT_EP 0x82

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

//Unknown packets
static unsigned char unk_1[] =  {0x17, 0x01};
static unsigned char unk_2[] =  {0x12, 0x01};
static unsigned char unk_3[] =  {0x13, 0x01};
static unsigned char unk_4[] =  {0x19, 0x15, 0x10, 0x40, 0x00};
static unsigned char unk_5[] =  {0x23, 0x40, 0x1c, 0x5e, 0x00, 0x00};
static unsigned char unk_6[] =  {0x23, 0x40, 0x1d, 0x01, 0x00, 0x00};
static unsigned char unk_7[] =  {0x19, 0x05, 0x10, 0x10, 0x00};
static unsigned char unk_8[] =  {0x19, 0x03, 0x03, 0x00, 0x00};


static bool ir_on = true;

static dev_found device = NONE;

static tir_interface tir2;
static tir_interface tir3;
static tir_interface tir4;
static tir_interface tir5;
static tir_interface *tir_iface = NULL;

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


static bool read_status_tir(tir_status_t *status)
{
  assert(status != NULL);
  size_t t;
  if(!ltr_int_send_data(Get_status, sizeof(Get_status))){
    ltr_int_log_message("Couldn't send status request!\n");
    return false;
  }
  int counter = 0;
  while(counter < 10){
    if(!ltr_int_receive_data(ltr_int_packet, sizeof(ltr_int_packet), &t, 2000)){
      ltr_int_log_message("Couldn't receive status!\n");
      return false;
    }
    if((ltr_int_packet[0] == 0x07) && (ltr_int_packet[1] == 0x20)){
      break;
    }
    ltr_int_usleep(10000);
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
  if(!ltr_int_send_data(unk_1, sizeof(unk_1))){
    ltr_int_log_message("Couldn't send config data request!\n");
    return false;
  }
  ltr_int_usleep(6000);

  int counter = 0;
  while(counter < 10){
    if(!ltr_int_receive_data(ltr_int_packet, sizeof(ltr_int_packet), &t, 2000)){
      ltr_int_log_message("Couldn't receive status!\n");
      return false;
    }
    if((ltr_int_packet[0] == 0x09) && (ltr_int_packet[1] == 0x40)){
      break;
    }
    ltr_int_usleep(10000);
    counter++;
  }
  return true;
}


static bool read_rom_data_tir()
{
  size_t t;
  if(!ltr_int_send_data(Get_conf, sizeof(Get_conf))){
    ltr_int_log_message("Couldn't send config data request!\n");
    return false;
  }
  int counter = 0;
  while(counter < 10){
    if(!ltr_int_receive_data(ltr_int_packet, sizeof(ltr_int_packet), &t, 2000)){
      ltr_int_log_message("Couldn't receive status!\n");
      return false;
    }
    //Tir4/5 0x14, Tir3 0x09...
    if(((ltr_int_packet[0] == 0x14) || (ltr_int_packet[0] == 0x09)) && (ltr_int_packet[1] == 0x40)){
      break;
    }
    ltr_int_usleep(10000);
    counter++;
  }
  return true;
}



static bool load_firmware(firmware_t *fw)
{
  assert(fw != NULL);
  gzFile *f;
  unsigned int tsize = FW_SIZE_INCREMENT;
  unsigned char *tbuf = ltr_int_my_malloc(tsize);
  unsigned char *ptr = tbuf;
  unsigned int read = 0;
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
  
  if(!ltr_int_send_data(Fpga_init, sizeof(Fpga_init))){
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

    if(!ltr_int_send_data(buf, len + 2)){
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
  if(!ltr_int_send_data(msg, sizeof(msg))){
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
  return ltr_int_send_data(Fifo_flush,sizeof(Fifo_flush));
}

bool ltr_int_set_threshold_tir(unsigned int val)
{
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
  return ltr_int_send_data(pkt, pkt_len);
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
  return ltr_int_send_data(pkt, sizeof(pkt));
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
  if(ltr_int_tir_get_status_indication()){
    return tir_iface->set_status_led_tir(running);
  }
  return true;
}

static bool control_status_led_tir(bool status1, bool status2)
{
  unsigned char pkt[] =  {0x19, 0x04, 0x10, 0x00, 0x00};
  pkt[3] = ltr_int_tir_get_status_brightness();
  pkt[4] = (status1 ? 0x20 : 0) | (status2 ? 0x02 : 0);
  return ltr_int_send_data(pkt, sizeof(pkt));
}

static bool turn_off_status_led_tir5()
{
  unsigned char pkt[] =  {0x19, 0x04, 0x10, 0x00, 0x00};
  return ltr_int_send_data(pkt, sizeof(pkt));
}

static bool control_ir_led_tir(bool ir)
{
  unsigned char pkt[] =  {0x19, 0x09, 0x10, 0x00, 0x00};
  int ir_brightness = ltr_int_tir_get_ir_brightness();
  pkt[3] = ir ? ir_brightness : 0;
  pkt[4] = ir ? 0x01 : 0;
  return ltr_int_send_data(pkt, sizeof(pkt));
}

static bool set_exposure(unsigned int exp)
{
  unsigned char Set_exposure_h[] =  {0x23, 0x42, 0x08, 0x01, 0x00, 0x00};
  unsigned char Set_exposure_l[] =  {0x23, 0x42, 0x10, 0x8F, 0x00, 0x00};
  
  Set_exposure_h[3] = exp >> 8;
  Set_exposure_l[3] = exp & 0xFF;
  return ltr_int_send_data(Set_exposure_h, sizeof(Set_exposure_h)) && 
    ltr_int_send_data(Set_exposure_l, sizeof(Set_exposure_l));
  
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
  ltr_int_send_data(Camera_stop,sizeof(Camera_stop));
  ltr_int_usleep(70000);
  turn_led_off_tir(TIR_LED_IR);
  return true;
}


static bool stop_camera_tir3()
{
  ltr_int_log_message("Stopping TIR3 camera!\n");
  ltr_int_send_data(Video_off,sizeof(Video_off));
  ltr_int_usleep(2000);
  ltr_int_send_data(Video_off,sizeof(Video_off));
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_IR);
  ltr_int_usleep(2000);
  flush_fifo_tir();
  ltr_int_usleep(2000);
  ltr_int_send_data(Camera_stop,sizeof(Camera_stop));
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
  ltr_int_send_data(Video_off,sizeof(Video_off));
  flush_fifo_tir();
  turn_led_off_tir(TIR_LED_IR);
  turn_led_off_tir(TIR_LED_RED);
  turn_led_off_tir(TIR_LED_GREEN);
  turn_led_off_tir(TIR_LED_BLUE);
  ltr_int_send_data(Camera_stop,sizeof(Camera_stop));
  ltr_int_usleep(50000);
  return true;
}

static bool stop_camera_tir5()
{
  turn_off_status_led_tir5();
  control_ir_led_tir(false);
  ltr_int_usleep(50000);
  ltr_int_send_data(Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(unk_2,sizeof(unk_2));
  ltr_int_send_data(unk_3,sizeof(unk_3));
  ltr_int_send_data(Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  ltr_int_send_data(Video_off,sizeof(Video_off));
  ltr_int_usleep(50000);
  flush_fifo_tir();
  ltr_int_send_data(Camera_stop,sizeof(Camera_stop));
  turn_off_status_led_tir5();
  ltr_int_usleep(50000);
  turn_off_status_led_tir5();
  control_ir_led_tir(false);
  turn_led_off_tir(TIR_LED_IR);
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
  ltr_int_send_data(Video_on,sizeof(Video_on));
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
  ltr_int_send_data(Camera_stop,sizeof(Camera_stop));
  ltr_int_usleep(2000);
  if(ir_on){ 
    turn_led_on_tir(TIR_LED_IR);
    ltr_int_usleep(2000);
  }
  ltr_int_send_data(Video_on,sizeof(Video_on));
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
  ltr_int_send_data(Video_on,sizeof(Video_on));
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
  ltr_int_send_data(Video_on,sizeof(Video_on));
  if(ir_on){ 
    control_ir_led_tir(true);
  }else{
    control_ir_led_tir(false);
  }
  if(ltr_int_tir_get_status_indication()) 
    set_status_led_tir5(true);
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
  ltr_int_send_data(Video_off,sizeof(Video_off));
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
  ltr_int_receive_data(ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  ltr_int_receive_data(ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  ltr_int_receive_data(ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
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
  ltr_int_receive_data(ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  ltr_int_receive_data(ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  ltr_int_receive_data(ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  if(!read_rom_data_tir3()){
    return false;
  }
  ltr_int_usleep(70000);
  ltr_int_set_threshold_tir(0x8c);
  ltr_int_usleep(70000);
  ltr_int_send_data(unk_5,sizeof(unk_5));
  ltr_int_usleep(2000);
  ltr_int_send_data(unk_6,sizeof(unk_6));
  ltr_int_usleep(2000);
  ltr_int_set_threshold_tir(0xd0);
  ltr_int_usleep(2000);
  ltr_int_send_data(unk_8,sizeof(unk_8));
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
  ltr_int_receive_data(ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  ltr_int_receive_data(ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
  ltr_int_receive_data(ltr_int_packet, sizeof(ltr_int_packet), &t, 100);
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
    return false;
  }
  
  if(status.fw_cksum != firmware.cksum){
    ltr_int_log_message("Firmware not loaded correctly!");
    return false;
  }
  
  if(status.cfg_flag == 1){
    ltr_int_send_data(Cfg_reload,sizeof(Cfg_reload));
    while(status.cfg_flag != 2){
      if(!read_status_tir(&status)){
	return false;
      }
    }
    if((device == TIR5)||(device == TIR5V2)){
      ltr_int_send_data(Set_ir_brightness,sizeof(Set_ir_brightness));
      set_exposure(0x18F);
    }
    ltr_int_set_threshold_tir(0x82);
  }else if(status.cfg_flag != 2){
    ltr_int_log_message("TIR configuration problem!\n");
    return false;
  }
  
  
  if((device == TIR5)||(device == TIR5V2)){
    ltr_int_send_data(Precision_mode,sizeof(Precision_mode));
  }
  
  free(firmware.firmware);
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
  ltr_int_send_data(unk_7,sizeof(unk_7));
  flush_fifo_tir();
  ltr_int_send_data(Camera_stop,sizeof(Camera_stop));
  read_status_tir(&status);
  ltr_int_send_data(Cfg_reload,sizeof(Cfg_reload));
  ltr_int_set_threshold_tir(0x96);
  set_exposure(0x18F);
  control_ir_led_tir(true);
  control_status_led_tir(false, false);
  control_status_led_tir(false, false);
  ltr_int_send_data(Precision_mode,sizeof(Precision_mode));
  control_status_led_tir(false, false);
  control_ir_led_tir(true);
  ltr_int_set_threshold_tir(0x76);
  set_exposure(0x15E);
  control_status_led_tir(false, false);
  ltr_int_send_data(Precision_mode,sizeof(Precision_mode));
  flush_fifo_tir();
  ltr_int_send_data(Camera_stop,sizeof(Camera_stop));
  control_ir_led_tir(true);
  ltr_int_send_data(Video_on,sizeof(Video_on));
  return true;
}

bool init_camera_tir(bool force_fw_load, bool p_ir_on)
{
  assert(tir_iface != NULL);
  return tir_iface->init_camera_tir(force_fw_load, p_ir_on);
}

bool ltr_int_open_tir(bool force_fw_load, bool ir_on)
{
  if(!ltr_int_init_usb()){
    ltr_int_log_message("Init failed!\n");
    return false;
  }
  if((device = ltr_int_find_tir()) == NONE){
    ltr_int_log_message("Tir not found!\n");
    return false;
  }
  
  //Tir3 uses endpoint 2, while Tir4 and 5 use endpoint 1!
  int in_ep = (device <= TIR3) ? TIR3_IN_EP : TIR_IN_EP;
  if(!ltr_int_prepare_device(TIR_CONFIGURATION, TIR_INTERFACE, TIR_OUT_EP, in_ep)){
    ltr_int_log_message("Couldn't prepare!\n");
    return false;
  }
  
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
    default:
      ltr_int_log_message("No device!\n");
      return false;
      break;
  }
  if(!init_camera_tir(force_fw_load, ir_on)){
    return false;
  }
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
  return true;
}

static bool close_camera_tir3(){
  ltr_int_log_message("Closing TIR3 camera!\n");
  stop_camera_tir();
  ltr_int_send_data(unk_4,sizeof(unk_4));
  ltr_int_usleep(25000);
  turn_led_off_tir(TIR_LED_RED);
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_GREEN);
  ltr_int_usleep(2000);
  turn_led_off_tir(TIR_LED_BLUE);
  ltr_int_usleep(2000);
  ltr_int_finish_usb(TIR_INTERFACE);
  return true;
}

static bool close_camera_tir4(){
  stop_camera_tir();
  ltr_int_finish_usb(TIR_INTERFACE);
  return true;
}

static bool close_camera_tir5()
{
  stop_camera_tir();
  ltr_int_set_threshold_tir(0x96);
  set_exposure(0x18F);
  control_ir_led_tir(true);
  control_status_led_tir(false, false);
  control_status_led_tir(false, false);
  ltr_int_send_data(Precision_mode,sizeof(Precision_mode));
  turn_led_off_tir(TIR_LED_IR);
  if(!ltr_int_send_data(Fpga_init, sizeof(Fpga_init))){
    ltr_int_log_message("Couldn't init fpga!\n");
  }
  ltr_int_finish_usb(TIR_INTERFACE);
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
  info->width = 710;
  info->height = 2 * 288;
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

char *ltr_int_find_firmware(dev_found device)
{
  const char *fw_file;
  switch(device){
    case TIR3:
    case TIR2:
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


