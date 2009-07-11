#include <stdio.h>
#include <stdint.h>
#include "tir4_driver.h"

#define TIR_VENDOR_ID 0x131d
#define TIR4_PRODUCT_ID 0x0156
#define CROPPED_NUM_VLINES 288
#define CROPPED_NUM_HPIX 710
#define RAW_NUM_VLINES 512
#define RAW_NUM_HPIX 1024
#define FRAME_QUEUE_MAX_DEPTH 2

/* private Constants */
#define NOP_MSGLEN 0XEF
#define TBD0_MSGLEN 0X07
#define TBD0_MSGID 0X20
#define VALID_MIN_MSGLEN 0x02
#define VALID_MAX_MSGLEN 0x3E
#define VALID_MSGID 0x1c
#define DEVICE_STRIPE_LEN 4
#define VSYNC_DEVICE_STRIPE {0x00, 0x00, 0x00, 0x00}
#define STRIPE_LEN 3
#define TIR_INTERFACE_ID 0
#define TIR_BULK_IN_EP 0x82
#define TIR_BULK_OUT_EP 0x01
#define TIR_CONFIGURATION 0x01
#define TIR_ALTINTERFACE 0x00
#define LINE_NUM_0X100_BIT_MASK 0X20
#define START_PIX_0X100_BIT_MASK 0X80
#define START_PIX_0X200_BIT_MASK 0X10
#define STOP_PIX_0X100_BIT_MASK 0X40
#define STOP_PIX_0X200_BIT_MASK 0X08
#define BULK_READ_SIZE 0X4000
#define BULK_READ_TIMEOUT 20   /* in milliseconds */
#define BULK_WRITE_TIMEOUT 1000 /* in milliseconds */
#define TIR_LED_MSGID 0x10
#define TIR_LED_MSGLEN 0x03
#define TIR_IR_LED_BIT_MASK 0x80
#define TIR_GREEN_LED_BIT_MASK 0x20
#define TIR_RED_LED_BIT_MASK 0x10
#define TIR_BLUE_LED_BIT_MASK 0x40
#define TIR_ALL_LED_BIT_MASK TIR_IR_LED_BIT_MASK | TIR_GREEN_LED_BIT_MASK | TIR_RED_LED_BIT_MASK | TIR_BLUE_LED_BIT_MASK
#define READ_DISCONNECT_TIMEOUT 2 /* in seconds */
#define MAX_CONFIG_PACKETSIZE 64
char *bulk_config_data_filename = "bulk_config_data.bin";

/* private Static members */
static uint16_t vline_offset = 12;
static uint16_t hpix_offset = 80;
static bool crop_frames = true;
static bool ir_led_on = false;
static bool green_led_on = false;
static bool red_led_on = false;
static bool blue_led_on = false;

/* globals */
struct usb_dev_handle *tir4_handle;

/* private function prototypes */
void tir4_set_configuration(void);
void tir4_set_altinterface(void);
void tir4_claim(void);
void tir4_release(void);
void tir4_close(void);
struct usb_device *tir4_find_device(uint16_t, uint16_t);
void tir4_error_alert(char *);
void tir4_write_bulk_config_data(void);
void set_led_worker(uint8_t, uint8_t);
void set_all_led_off();
void set_ir_led_on(bool);
void set_green_led_on(bool);
void set_red_led_on(bool);
void set_blue_led_on(bool);
bool is_ir_led_on(void);
bool is_green_led_on(void);
bool is_red_led_on(void);
bool is_blue_led_on(void);

void tir4_init(void)
{
  struct usb_device *tir4_device;

  /* required libusb initialization */
  usb_init();
  /* This tells libusb to go find all usb busses 
   * the output gets stored in the extern global usb_busses
   * declared in usb.h */
  usb_find_busses();
  /* This tells libusb to go find all devices on the busses 
   * This will to further populate the usb.h extern global 
   * usb_busses structure */
  usb_find_devices();

  /* find the tir4 */
  tir4_device = tir4_find_device(TIR_VENDOR_ID,TIR4_PRODUCT_ID);
  if(!tir4_device) { 
    tir4_error_alert("Can't locate TIR4 device\n");
    exit(1);
  }

  /* open the tir4 */
  tir4_handle = usb_open(tir4_device);

  /* claim the tir4 */
  tir4_claim();
  
  /* this makes it work */
  tir4_set_altinterface();

  /* again this is strange, but it seems to work */
  tir4_release();
  tir4_set_configuration();
  tir4_claim();
  tir4_set_altinterface();
  
  tir4_write_bulk_config_data();
  set_all_led_off();
  set_ir_led_on(true);
}

enum bulk_write_state {
  reading_checksum,
  verifying_checksum,
  skipping_checksum,
  reading_data_byte0,
  reading_data_byte1
};

void tir4_write_bulk_config_data(void)
{
  FILE *bulkfp;
  int fgetc_result;
  uint8_t pendingbyte;
  uint32_t filechecksum=0;
  uint32_t computedchecksum=0;
  uint8_t bytecount=0;
  char configpacket[MAX_CONFIG_PACKETSIZE];

  enum bulk_write_state state = reading_checksum;

  bulkfp = fopen(bulk_config_data_filename, "rb");
  if (bulkfp == NULL) {
    tir4_error_alert("Unable to open the tir4 bulk config datafile.\n");
    tir4_shutdown();
    exit(1);
  }
 
  while ((fgetc_result = fgetc(bulkfp)) != EOF){
    switch (state) {
    case reading_checksum:
      filechecksum |= fgetc_result<<(8*bytecount);
      bytecount++;
      if (bytecount == 4) {
        state = verifying_checksum;
      }
      break;
    case verifying_checksum:
      computedchecksum += fgetc_result;
      break;
    default:
      tir4_error_alert("Invalid state while reading bulk config datafile.\n");
      tir4_shutdown();
      exit(1);
    }
  }
  computedchecksum = ~computedchecksum;
  if (computedchecksum != filechecksum) {
    printf("computedchecksum: 0x%08x\n",computedchecksum);
    printf("filechecksum: 0x%08x\n",filechecksum);
    tir4_error_alert("Bulk config datafile failed checksum.\n");
    tir4_shutdown();
    exit(1);
  }
  rewind(bulkfp);
  state = skipping_checksum;

  bytecount = 0;
  while ((fgetc_result = fgetc(bulkfp)) != EOF){
    switch (state) {
    case skipping_checksum:
      bytecount++;
      if (bytecount == 4) {
        bytecount = 0;
        state = reading_data_byte0;
      }
      break;
    case reading_data_byte0:
      pendingbyte = (uint8_t) fgetc_result;
      state = reading_data_byte1;
      break;
    case reading_data_byte1:
      if (fgetc_result != 0xFF) {
        configpacket[bytecount] = (char) pendingbyte;
        bytecount++;
      }
      else {
/*         int i; */
/*         for(i=0;i<bytecount;i++) { */
/*           printf("0x%02x ", (uint8_t) configpacket[i]); */
/*         } */
/*         printf("\n"); */
        usb_bulk_write(tir4_handle,
                       TIR_BULK_OUT_EP,
                       configpacket,
                       bytecount,
                       BULK_WRITE_TIMEOUT);
        bytecount = 0;
      }
      state = reading_data_byte0;
      break;
    default:
      tir4_error_alert("Invalid state while reading bulk config datafile.\n");
      tir4_shutdown();
      exit(1);
    }
  }
  fclose(bulkfp);
}

void tir4_shutdown(void)
{
  set_all_led_off();
  tir4_release();
  tir4_close();
}

void tir4_set_good_indication(bool arg)
{
  set_green_led_on(arg);
  set_red_led_on(!arg);
}

void tir4_set_configuration(void)
{
  int ret;
  ret = usb_set_configuration(tir4_handle, TIR_CONFIGURATION);
  if (ret < 0) {
    tir4_error_alert("Unable to set the configuration of the TIR4 device.\n");
    tir4_shutdown();
    exit(1);
  }  
}  

void tir4_set_altinterface(void)
{
  int ret;
  ret = usb_set_altinterface(tir4_handle, TIR_ALTINTERFACE);
  if (ret < 0) {
    tir4_error_alert("Unable to set the alt interface of the TIR4 device.\n");
    tir4_shutdown();
    exit(1);
  }  
}

void tir4_claim(void)
{
  int ret;
  ret = usb_claim_interface(tir4_handle, TIR_INTERFACE_ID);
  if (ret < 0) {
    tir4_error_alert("Unable to claim the TIR4 device.\nThis is most likely a permissions problem.\nRunning this app sudo may be a quick fix.\n");
    tir4_release();
    exit(1);
  }  
}
/* search the bus for our desired device */
struct usb_device *tir4_find_device(uint16_t vid, uint16_t pid)
{
  struct usb_bus *bus;
  struct usb_device *dev;
  /* usb busses is a usb.h global var */
  for (bus = usb_busses; bus;
       bus = bus->next) {
    for (dev = bus->devices; dev;
         dev = dev->next) {
      if ((dev->descriptor.idVendor ==
           vid) &&
          (dev->descriptor.idProduct ==
           pid))
        return dev;
    }
  }
  return NULL;
}

void tir4_close(void) 
{
  int ret;
  ret = usb_close(tir4_handle);
  if (ret < 0) {
    tir4_error_alert("failed to close TIR4 interface.\n");
    tir4_release();
    exit(1);
  }
}

void tir4_release(void) 
{
  int ret;
  ret = usb_release_interface(tir4_handle, 0);
  if (ret < 0) {
    tir4_error_alert("failed to release TIR4 interface.\n");
    exit(1);
  }
}

void tir4_error_alert(char *message)
{
  fprintf(stderr, message);
}

void set_led_worker(uint8_t cmd, uint8_t mask)
{
  char msg[TIR_LED_MSGLEN];
  msg[0] = TIR_LED_MSGID;
  msg[1] = cmd;
  msg[2] = mask;

  usb_bulk_write(tir4_handle,
                 TIR_BULK_OUT_EP,
                 msg,
                 TIR_LED_MSGLEN,
                 BULK_WRITE_TIMEOUT);
}

void set_all_led_off(void)
{
  set_led_worker(0,TIR_ALL_LED_BIT_MASK);
  ir_led_on = false;
  green_led_on = false;
  red_led_on = false;
  blue_led_on = false;
}

void set_ir_led_on(bool arg)
{
  if (arg) {
    set_led_worker(TIR_IR_LED_BIT_MASK,TIR_IR_LED_BIT_MASK);
  }
  else {
    set_led_worker(0,TIR_IR_LED_BIT_MASK);
  }
  ir_led_on = arg;
}

void set_green_led_on(bool arg)
{
  if (arg) {
    set_led_worker(TIR_GREEN_LED_BIT_MASK,TIR_GREEN_LED_BIT_MASK);
  }
  else {
    set_led_worker(0,TIR_GREEN_LED_BIT_MASK);
  }
  green_led_on = arg;
}

void set_red_led_on(bool arg)
{
  if (arg) {
    set_led_worker(TIR_RED_LED_BIT_MASK,TIR_RED_LED_BIT_MASK);
  }
  else {
    set_led_worker(0,TIR_RED_LED_BIT_MASK);
  }
  red_led_on = arg;
}

void set_blue_led_on(bool arg)
{
  if (arg) {
    set_led_worker(TIR_BLUE_LED_BIT_MASK,TIR_BLUE_LED_BIT_MASK);
  }
  else {
    set_led_worker(0,TIR_BLUE_LED_BIT_MASK);
  }
  blue_led_on = arg;
}

bool is_ir_led_on(void)
{
  return ir_led_on;
}

bool is_green_led_on(void)
{
  return green_led_on;
}

bool is_red_led_on(void)
{
  return red_led_on;
}

bool is_blue_led_on(void)
{
  return blue_led_on;
}

