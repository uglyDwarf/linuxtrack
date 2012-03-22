#ifndef TIRHW__H
#define TIRHW__H

#include "usb_ifc.h"

#define TIR_LED_RED    0x10
#define TIR_LED_GREEN  0x20
#define TIR_LED_BLUE   0x40
#define TIR_LED_IR     0x80

#define TIR_PACKET_SIZE 16384
extern unsigned char ltr_int_packet[TIR_PACKET_SIZE];

typedef struct {
  //Width and height of image TIRx sees
  unsigned int width;
  unsigned int height;
  //height squash factor - TIR4 has width upsampled by factor of 2,
  // and this is the compensation factor
  float hf;
  dev_found dev_type;
}tir_info;

void ltr_int_get_tir_info(tir_info *info);
char *ltr_int_find_firmware(dev_found device);

typedef bool (*stop_camera_tir_fun)();
typedef bool (*start_camera_tir_fun)();
typedef bool (*init_camera_tir_fun)(bool force_fw_load, bool p_ir_on);
typedef void (*get_tir_info_fun)(tir_info *info);
typedef bool (*close_camera_tir_fun)();
typedef bool (*set_status_led_tir_fun)(bool state);

typedef struct {
  stop_camera_tir_fun stop_camera_tir;
  start_camera_tir_fun start_camera_tir;
  init_camera_tir_fun init_camera_tir;
  close_camera_tir_fun close_camera_tir;
  get_tir_info_fun get_tir_info;
  set_status_led_tir_fun set_status_led_tir;
} tir_interface;


bool ltr_int_open_tir(bool force_fw_load, bool ir_on);
void ltr_int_get_tir_info(tir_info *info);
bool ltr_int_pause_tir();
bool ltr_int_resume_tir();
bool ltr_int_close_tir();
bool ltr_int_set_threshold_tir(unsigned int val);


#endif

