#ifndef TIRHW__H
#define TIRHW__H

#define TIR_LED_RED    0x10
#define TIR_LED_GREEN  0x20
#define TIR_LED_BLUE   0x40
#define TIR_LED_IR     0x80

unsigned char packet[4096];

extern bool turn_led_off_tir(unsigned char leds);
extern bool turn_led_on_tir(unsigned char leds);
extern bool stop_camera_tir();
extern bool start_camera_tir();
extern bool init_camera_tir(char data_path[], bool force_fw_load, bool p_ir_on);
extern void get_res_tir(unsigned int *w, unsigned int *h);
extern void switch_green(bool state);
extern void switch_blue(bool state);
extern void switch_red(bool state);
extern void switch_ir(bool state);


#endif
