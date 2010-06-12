#ifndef TIRHW__H
#define TIRHW__H

#define TIR_LED_RED    0x10
#define TIR_LED_GREEN  0x20
#define TIR_LED_BLUE   0x40
#define TIR_LED_IR     0x80

unsigned char ltr_int_packet[4096];

void ltr_int_get_res_tir(unsigned int *w, unsigned int *h, float *hf);

typedef bool (*stop_camera_tir_fun)();
typedef bool (*start_camera_tir_fun)();
typedef bool (*init_camera_tir_fun)(const char data_path[], bool force_fw_load, bool p_ir_on);
typedef void (*get_res_tir_fun)(unsigned int *w, unsigned int *h, float *hf);
typedef bool (*close_camera_tir_fun)();
typedef bool (*set_status_led_tir_fun)(bool state);

typedef struct {
  stop_camera_tir_fun stop_camera_tir;
  start_camera_tir_fun start_camera_tir;
  init_camera_tir_fun init_camera_tir;
  close_camera_tir_fun close_camera_tir;
  get_res_tir_fun get_res_tir;
  set_status_led_tir_fun set_status_led_tir;
} tir_interface;

//extern tir_interface tir4;
//extern tir_interface tir5;

bool ltr_int_open_tir(const char data_path[], bool force_fw_load, bool ir_on);
void ltr_int_get_res_tir();
bool ltr_int_pause_tir();
bool ltr_int_resume_tir();
bool ltr_int_close_tir();
bool ltr_int_set_threshold_tir(unsigned int val);

#endif

