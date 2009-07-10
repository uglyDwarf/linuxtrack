#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "tir4_driver.h"
#include "bulk_config_data.h"

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
#define TIR_IR_LED_BIT_MASK 0x80
#define TIR_GREEN_LED_BIT_MASK 0x20
#define TIR_RED_LED_BIT_MASK 0x10
#define TIR_BLUE_LED_BIT_MASK 0x40
#define TIR_ALL_LED_BIT_MASK TIR_IR_LED_BIT_MASK | TIR_GREEN_LED_BIT_MASK | TIR_RED_LED_BIT_MASK | TIR_BLUE_LED_BIT_MASK
#define READ_DISCONNECT_TIMEOUT 2 /* in seconds */

/* private Static members */
uint16_t vline_offset = 12;
uint16_t hpix_offset = 80;
bool crop_frames = true;

/* globals */
struct usb_dev_handle *tir4_handle;

void init(void)
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
  tir4_device = find_device(TIR_VENDOR_ID,TIR4_PRODUCT_ID);
  if(!tir4_device) { 
    error_alert("Can't locate TIR4 device\n");
    exit(1);
  }

  /* open the tir4 */
  tir4_handle = usb_open(tir4_device);

  /* claim the tir4 */
  claim_tir4();
  
  /* this makes it work */
  set_altinterface_tir4();
  
/*   int ret; */
/*   char buf[65535]; */
/*   ret = usb_get_descriptor(tir4_handle, */
/*                            0x0000002,  /\* type *\/ */
/*                            0x0000000,  /\* index *\/ */
/*                            buf,        /\* buffer *\/ */
/*                            0x0000009); /\* length *\/ */

  /* again this is strange, but it seems to work */
  release_tir4();
  set_configuration_tir4();
  claim_tir4();
  set_altinterface_tir4();
  
  shutdown();
}

void shutdown(void)
{
  release_tir4();
  close_tir4();
}

void set_configuration_tir4()
{
  int ret;
  ret = usb_set_configuration(tir4_handle, TIR_CONFIGURATION);
  if (ret < 0) {
    error_alert("Unable to set the configuration of the TIR4 device.\n");
    shutdown();
    exit(1);
  }  
}  

void set_altinterface_tir4(void)
{
  int ret;
  ret = usb_set_altinterface(tir4_handle, TIR_ALTINTERFACE);
  if (ret < 0) {
    error_alert("Unable to set the alt interface of the TIR4 device.\n");
    shutdown();
    exit(1);
  }  
}

void claim_tir4(void)
{
  int ret;
  ret = usb_claim_interface(tir4_handle, TIR_INTERFACE_ID);
  if (ret < 0) {
    error_alert("Unable to claim the TIR4 device.\nThis is most likely a permissions problem.\nRunning this app sudo may be a quick fix.\n");
    release_tir4();
    exit(1);
  }  
}
/* search the bus for our desired device */
struct usb_device *find_device(uint16_t vid, uint16_t pid)
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

void close_tir4(void) 
{
  int ret;
  ret = usb_close(tir4_handle);
  if (ret < 0) {
    error_alert("failed to close TIR4 interface.\n");
    release_tir4();
    exit(1);
  }
}

void release_tir4(void) 
{
  int ret;
  ret = usb_release_interface(tir4_handle, 0);
  if (ret < 0) {
    error_alert("failed to release TIR4 interface.\n");
    exit(1);
  }
}

void error_alert(char *message)
{
  fprintf(stderr, message);
}

int main(int argc, char **argv) {
  init();
  return 0;
}
