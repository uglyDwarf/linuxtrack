#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "tir4_driver.h"
#include "utils.h"

/*********************/
/* private Constants */
/*********************/
#define TIR_VENDOR_ID 0x131d
#define TIR4_PRODUCT_ID 0x0156
/* #define CROPPED_NUM_VLINES TIR4_VERTICAL_SQUASHED_RESOLUTION */
/* #define CROPPED_NUM_HPIX TIR4_HORIZONTAL_RESOLUTION */
/* #define RAW_NUM_VLINES 512 */
/* #define RAW_NUM_HPIX 1024 */
#define NOP_MSGLEN 0XEF
#define TBD0_MSGLEN 0X07
#define TBD0_MSGID 0X20
#define VALID_MIN_MSGLEN 0x02
#define VALID_MAX_MSGLEN 0x3E
#define VALID_MSGID 0x1c
#define DEVICE_STRIPE_LEN 4
#define VSYNC_DEVICE_STRIPE_BYTE 0x00
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
#define BULK_READ_SIZE 64
#define BULK_READ_TIMEOUT 40   /* in milliseconds */
#define BULK_WRITE_TIMEOUT 1000 /* in milliseconds */
#define TIR_LED_MSGID 0x10
#define TIR_LED_MSGLEN 0x03
#define TIR_IR_LED_BIT_MASK 0x80
#define TIR_GREEN_LED_BIT_MASK 0x20
#define TIR_RED_LED_BIT_MASK 0x10
#define TIR_BLUE_LED_BIT_MASK 0x40
#define TIR_ALL_LED_BIT_MASK (TIR_IR_LED_BIT_MASK | TIR_GREEN_LED_BIT_MASK | TIR_RED_LED_BIT_MASK | TIR_BLUE_LED_BIT_MASK)
#define MAX_BULK_CONFIG_PACKETSIZE 64
#define BULK_CONFIG_INTERPACKET_SLEEP_TIME 800 /* microseconds? */
#define BITMAP_NUM_BYTES (TIR4_HORIZONTAL_RESOLUTION*TIR4_VERTICAL_RESOLUTION)
#define TIR4_LIBUSB_TIMEOUT_ERR -110
#define TIR4_LIBUSB_NO_DEVICE_ERR -19

/**********************/
/* private data types */
/**********************/
struct stripe_type {
  uint16_t vline;
  uint16_t hstart;
  uint16_t hstop;
};

struct stripelist_type {
  struct stripelist_iter *head;
  struct stripelist_iter *tail;
};

struct stripelist_iter {
  struct stripe_type stripe;
  struct stripelist_iter *next;
  struct stripelist_iter *prev;
};

struct protoblob_type {
  /* total # pixels area, used for sorting/scoring blobs */
  unsigned int cumulative_area;
  /* used to calculate x */
  unsigned int cumulative_2x_hcenter_area_product;
  /* used to calculate y */
  unsigned int cumulative_linenum_area_product;
  /* used to identify blobs */
  struct stripelist_type stripes;
};

struct protobloblist_type {
  unsigned int length;
  struct protobloblist_iter *head;
  struct protobloblist_iter *tail;
};

struct protobloblist_iter {
  struct protoblob_type pblob;
  struct protobloblist_iter *next;
  struct protobloblist_iter *prev;
};

struct framelist_type {
  struct framelist_iter *head;
  struct framelist_iter *tail;
};

struct framelist_iter {
  struct frame_type frame;
  struct framelist_iter *next;
  struct framelist_iter *prev;
};

enum bulk_write_state {
  reading_checksum,
  verifying_checksum,
  skipping_checksum,
  reading_data_byte0,
  reading_data_byte1
};

typedef uint8_t device_stripe_type[DEVICE_STRIPE_LEN];

enum message_processor_state {
  awaiting_header_byte0=0,
  awaiting_header_byte1,
  processing_msg,
  chomping_msg
};

/**************************/
/* private Static members */
/**************************/
static char *bulk_config_data_filename = "bulk_config_data.bin";
/* notes:
   min vl: 12
   max vl: 299
   min hp: 81
   max hp: 789
*/
static uint16_t vline_offset = 12;
static uint16_t hpix_offset = 80;
static bool ir_led_on = false;
static bool green_led_on = false;
static bool red_led_on = false;
static bool blue_led_on = false;
static struct usb_dev_handle *tir4_handle;
static char usb_read_buf[BULK_READ_SIZE];
static enum message_processor_state msgproc_state = awaiting_header_byte0;
static uint8_t msgproc_msglen;
static uint8_t msgproc_msgid;
static uint8_t msgproc_msgcnt;
static device_stripe_type msgproc_pending_device_stripe;
static uint8_t msgproc_substripe_index;
static struct protobloblist_type msgproc_open_blobs;
static struct protobloblist_type msgproc_closed_blobs;
static struct framelist_type master_framelist;
static bool usb_inited = false;
static uint16_t msgproc_stripe_minimum_vline;

/*******************************/
/* private function prototypes */
/*******************************/
int tir4_fatal(void);
int tir4_set_configuration(void);
int tir4_set_altinterface(void);
int tir4_claim(void);
int tir4_release(void);
int tir4_close(void);
struct usb_device *tir4_find_device(uint16_t, uint16_t);
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
void msgproc_init(void);
void msgproc_add_byte(uint8_t b, enum cal_operating_mode opmode);
struct stripe_type msgproc_convert_device_stripe(device_stripe_type ds);
void msgproc_add_stripe(struct stripe_type s);
void msgproc_close_all_open_blobs(void);
uint16_t pixel_crop(uint16_t inpixel,
                    uint16_t offset,
                    uint16_t limitpixel);
bool stripe_is_hcontact(struct stripe_type s0,
                        struct stripe_type s1);
void stripe_draw(struct stripe_type s, char *bitmap);
void stripe_print(struct stripe_type s);
void protoblob_init(struct protoblob_type *pb);
float point_crop(float inpoint,
                 float offset,
                 float limitpoint);
void protoblob_addstripe(struct protoblob_type *pb,
                         struct stripe_type s);
void protoblob_merge(struct protoblob_type *pb0,
                     struct protoblob_type *pb1);
bool protoblob_is_contact(struct protoblob_type pb,
                          struct stripe_type s);
void protoblob_make_blob(struct protoblob_type *pb,
                         struct blob_type *b);
void protoblob_print(struct protoblob_type pbl);
void protobloblist_init(struct protobloblist_type *pbl);
void protobloblist_delete(struct protobloblist_type *pbl,
                          struct protobloblist_iter *pbli);
struct protobloblist_iter *protobloblist_get_iter(struct protobloblist_type *pbl);
struct protobloblist_iter *protobloblist_iter_next(struct protobloblist_iter *pbli);
bool protobloblist_iter_complete(struct protobloblist_iter *pbli);
void protobloblist_free(struct protobloblist_type *pbl);
void protobloblist_add_protoblob(struct protobloblist_type *pbl,
                                 struct protoblob_type b);
void protobloblist_pop_biggest_protoblob(struct protobloblist_type *pbl,
                                         struct protoblob_type *pb);
bool protobloblist_populate_frame(struct protobloblist_type *pbl,
                                  struct frame_type *f,
                                  enum cal_operating_mode opmode);
void protobloblist_draw(struct protobloblist_type *pbl,
                        char *bmp);
void protobloblist_print(struct protobloblist_type pbl);
void framelist_init(struct framelist_type *fl);
void framelist_pop(struct framelist_type *fl,
                   struct framelist_iter *fli,
                   struct frame_type *f);
struct framelist_iter *framelist_get_iter(struct framelist_type *fl);
struct framelist_iter *framelist_iter_next(struct framelist_iter *fli);
bool framelist_iter_complete(struct framelist_iter *fli);
void framelist_flush(struct camera_control_block *ccb,
                     struct framelist_type *fl);
void framelist_add_frame(struct framelist_type *fl,
                         struct frame_type f);
void framelist_print(struct framelist_type *fl);
int tir4_do_read_and_process(struct camera_control_block *ccb);
int tir4_populate_frame(struct frame_type *f);
bool tir4_is_frame_available(void);

/*************/
/* interface */
/*************/

dev_interface tir4_interface = {
  .device_init = tir4_init,
  .device_shutdown = tir4_shutdown,
  .device_suspend = tir4_suspend,
  .device_change_operating_mode = tir4_change_operating_mode,
  .device_wakeup = tir4_wakeup,
  .device_set_good_indication = tir4_set_good_indication,
  .device_get_frame = tir4_get_frame,
};



/************************/
/* function definitions */
/************************/
bool tir4_is_device_present(struct cal_device_type *cal_device)
{
  struct usb_device *tir4_device;

  /* required libusb initialization */
  if (!usb_inited) {
    usb_init();
    usb_inited = true;
  }
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

  return (tir4_device!=NULL);
}

int tir4_init(struct camera_control_block *ccb)
{
  struct usb_device *tir4_device;
  int retval;

  ccb->pixel_width = TIR4_HORIZONTAL_RESOLUTION;
  ccb->pixel_height = TIR4_VERTICAL_RESOLUTION;

  /* required libusb initialization */
  if (!usb_inited) {
    usb_init();
    usb_inited = true;
  }
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
    log_message("TIR4: Unable to locate TIR4 device\n");
    return CAL_UNABLE_TO_FIND_DEVICE_ERR;
  }

  /* open the tir4 */
  tir4_handle = usb_open(tir4_device);

  /* claim the tir4 */
  retval = tir4_claim();
  if (retval< 0) { return retval; }

  /* this makes it work */
  retval = tir4_set_altinterface();
  if (retval< 0) { return retval; }

  /* again this is strange, but it seems to work */
  tir4_release();
  retval = tir4_set_configuration();
  if (retval< 0) { return retval; }
  retval = tir4_claim();
  if (retval< 0) { return retval; }
  retval = tir4_set_altinterface();
  if (retval< 0) { return retval; }

  tir4_write_bulk_config_data();
  set_all_led_off();
  if (ccb->enable_IR_illuminator_LEDS) {
    set_ir_led_on(true);
  }

  msgproc_init();
  framelist_init(&master_framelist);

  ccb->state = active;

  return 0;
}

int tir4_fatal(void)
{
  set_all_led_off();
  tir4_release();
  tir4_close();
  return -1;
}

int tir4_shutdown(struct camera_control_block *ccb)
{
  framelist_flush(ccb,
                  &master_framelist);
  set_all_led_off();
  tir4_release();
  tir4_close();
  ccb->state = pre_init;
  return 0;
}

int tir4_suspend(struct camera_control_block *ccb)
{
  ccb->state = suspended;
  set_all_led_off();
  framelist_flush(ccb, &master_framelist);
  return 0;
}

int tir4_change_operating_mode(struct camera_control_block *ccb,
                               enum cal_operating_mode newmode)
{
  ccb->mode = newmode;
  if (!(ccb->state == suspended)) {
    log_message("TIR4: Attempted to change the operating mode of the TIR4 device while outside of suspended mode.\n");
    return CAL_INVALID_OPERATION_ERR;
  }
  return 0;
}

int tir4_wakeup(struct camera_control_block *ccb)
{
  if (ccb->enable_IR_illuminator_LEDS) {
    set_ir_led_on(true);
  }
  ccb->state = active;
  return 0;
}

int tir4_set_good_indication(struct camera_control_block *ccb,
                             bool arg)
{
  set_green_led_on(arg);
  set_red_led_on(!arg);
  return 0;
}

int tir4_get_frame(struct camera_control_block *ccb,
                   struct frame_type *f)
{
  int retval;
  /* FIXME: do a flush here if its too long
   * or make the queue fixed length! */
  while (!tir4_is_frame_available()) {
    retval=tir4_do_read_and_process(ccb);
    if (retval < 0) { return retval; }
  }
  tir4_populate_frame(f);
  return 0;
}

int tir4_do_read_and_process(struct camera_control_block *ccb)
{
  int numread;
  numread = usb_bulk_read(tir4_handle,
                          TIR_BULK_IN_EP,
                          usb_read_buf,
                          BULK_READ_SIZE,
                          BULK_READ_TIMEOUT);
  if (numread < 0) {
    if (numread == TIR4_LIBUSB_TIMEOUT_ERR) {
      log_message("TIR4: USB read timeout.");
    }
    else if (numread == TIR4_LIBUSB_NO_DEVICE_ERR) {
      log_message("TIR4: Camera disconnected.\n");
      return CAL_DISCONNECTED_ERR;
    }
    else {
    log_message("TIR4: Unknown error during read.  USB Errstring: %s\n",usb_strerror());
      return CAL_UNKNOWN_ERR;
    }
  }
  int i;
  for (i=0; i<numread; i++) {
    msgproc_add_byte((uint8_t) usb_read_buf[i],
                     ccb->mode);
  }
  return 0;
}

int tir4_populate_frame(struct frame_type *f)
{
  framelist_pop(&master_framelist,master_framelist.tail,f);
  return 0;
}

int tir4_set_configuration(void)
{
  int ret;
  ret = usb_set_configuration(tir4_handle, TIR_CONFIGURATION);
  if (ret < 0) {
    log_message("TIR4: Unable to set the configuration of the TIR4 device.\n");
    return CAL_DEVICE_OPEN_ERR;
  }
  return 0;
}

int tir4_set_altinterface(void)
{
  int ret;
  ret = usb_set_altinterface(tir4_handle, TIR_ALTINTERFACE);
  if (ret < 0) {
    log_message("TIR4: Unable to set the alt interface of the TIR4 device.\n");
    return CAL_DEVICE_OPEN_ERR;
  }
  return 0;
}

int tir4_claim(void)
{
  int ret;
  ret = usb_claim_interface(tir4_handle, TIR_INTERFACE_ID);
  if (ret < 0) {
    log_message("TIR4: Unable to claim the TIR4 device.\nThis is most likely a permissions problem.\nRunning this app sudo may be a quick fix.\n");
    return CAL_PROBABLE_PERMISSIONS_ERR;
  }
  return 0;
}

int tir4_release(void)
{
  int ret;
  ret = usb_release_interface(tir4_handle, 0);
  if (ret < 0) {
    log_message("TIR4: failed to release TIR4 interface.\n");
    return CAL_UNKNOWN_ERR;
  }
  return 0;
}

int tir4_close(void)
{
  int ret;
  ret = usb_close(tir4_handle);
  if (ret < 0) {
    log_message("TIR4: failed to close TIR4 interface.\n");
    return CAL_UNKNOWN_ERR;
  }
  return 0;
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

void tir4_write_bulk_config_data(void)
{
  FILE *bulkfp;
  int fgetc_result;
  uint8_t pendingbyte;
  uint32_t filechecksum=0;
  uint32_t computedchecksum=0;
  uint8_t bytecount=0;
  char configpacket[MAX_BULK_CONFIG_PACKETSIZE];

  enum bulk_write_state state = reading_checksum;

  bulkfp = fopen(bulk_config_data_filename, "rb");
  if (bulkfp == NULL) {
    log_message("TIR4: Unable to open the tir4 bulk config datafile.\n");
    tir4_fatal();
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
      log_message("TIR4: Invalid state while reading bulk config datafile.\n");
      tir4_fatal();
      exit(1);
    }
  }
  computedchecksum = ~computedchecksum;
  if (computedchecksum != filechecksum) {
    log_message("TIR4: computedchecksum: 0x%08x\n",computedchecksum);
    log_message("TIR4: filechecksum: 0x%08x\n",filechecksum);
    log_message("TIR4: Bulk config datafile failed checksum.\n");
    tir4_fatal();
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
        usb_bulk_write(tir4_handle,
                       TIR_BULK_OUT_EP,
                       configpacket,
                       bytecount,
                       BULK_WRITE_TIMEOUT);
        bytecount = 0;
        usleep(BULK_CONFIG_INTERPACKET_SLEEP_TIME);
      }
      state = reading_data_byte0;
      break;
    default:
      log_message("TIR4: Invalid state while reading bulk config datafile.\n");
      tir4_fatal();
      exit(1);
    }
  }
  fclose(bulkfp);
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

void msgproc_init(void)
{
  msgproc_state = awaiting_header_byte0;
  protobloblist_init(&msgproc_open_blobs);
  protobloblist_init(&msgproc_closed_blobs);
  msgproc_stripe_minimum_vline = 0;
}

void msgproc_add_byte(uint8_t b, enum cal_operating_mode opmode)
{
  int i;
  bool is_vsync, frame_was_made;
  struct frame_type f;
  f.bitmap = NULL;

  switch (msgproc_state) {
  case awaiting_header_byte0:
    msgproc_msglen = b;
    msgproc_state = awaiting_header_byte1;
    break;
  case awaiting_header_byte1:
    msgproc_msgid = b;
    /* if its a msg to skip */
    if (msgproc_msglen == NOP_MSGLEN) {
      msgproc_state = awaiting_header_byte0;
      /* chomp and continue AWAITING_HEADER */;
    }
    /* if it looks like a valid message  */
    else if ((msgproc_msglen <= VALID_MAX_MSGLEN) &&
             (msgproc_msglen >= VALID_MIN_MSGLEN) &&
             (msgproc_msgid == VALID_MSGID)) {
      msgproc_msgcnt = VALID_MIN_MSGLEN;
      msgproc_substripe_index = 0;
      msgproc_state = processing_msg;
    }
    /* if its a TBD message, we'll drop it */
    else if ((msgproc_msglen == TBD0_MSGLEN) &&
             (msgproc_msgid == TBD0_MSGID)) {
      msgproc_msgcnt = VALID_MIN_MSGLEN;
      msgproc_state = chomping_msg;
    }
    /* error case */
    else {
      /* maybe we're off by one?
       * drop one and try again */
      log_message("TIR4: Debug: Warning USB bad packet data: 0x%02x\n", msgproc_msglen);
      msgproc_msglen = b;
      msgproc_state = awaiting_header_byte1;
    }
    break;
  case processing_msg:
    /* first 3 bytes of the device stripe */
    if (msgproc_substripe_index < (DEVICE_STRIPE_LEN-1)) {
      msgproc_msgcnt++;
      if (msgproc_msgcnt >= msgproc_msglen) {
        free(f.bitmap);
        log_message("TIR4: Debug: USB packet ended midstripe. Packet dropped.\n");
        msgproc_state = awaiting_header_byte0;
      }
      else {
        msgproc_pending_device_stripe[msgproc_substripe_index] = b;
        msgproc_substripe_index++;
      }
    }
    /* last byte of the device stripe,
     * get it in and run with it */
    else {
      msgproc_pending_device_stripe[msgproc_substripe_index] = b;
      is_vsync = true;
      for (i=0;i<DEVICE_STRIPE_LEN;i++) {
        if (msgproc_pending_device_stripe[i] != VSYNC_DEVICE_STRIPE_BYTE) {
          is_vsync = false;
          break;
        }
      }
      if (!is_vsync) {
        struct stripe_type newstripe;
        msgproc_substripe_index = 0;
        newstripe = msgproc_convert_device_stripe(msgproc_pending_device_stripe);
        if (newstripe.vline < msgproc_stripe_minimum_vline) {
          log_message("TIR4: Warning: TIR4 out of order stripe detected; min_vline: %d\tstripe.vline: %d\n", msgproc_stripe_minimum_vline, newstripe.vline);

        }
        msgproc_add_stripe(newstripe);
      }
      else {
        msgproc_stripe_minimum_vline = 0;
        /* about done, just move any open to closed blobs,
         * and create the frame */
        msgproc_close_all_open_blobs();
        frame_was_made = protobloblist_populate_frame(&msgproc_closed_blobs,
                                                      &f,
                                                      opmode);
        if (frame_was_made) {
          framelist_add_frame(&master_framelist, f);
        }
        protobloblist_free(&msgproc_closed_blobs);
      }
      msgproc_msgcnt++;
      if (msgproc_msgcnt >= msgproc_msglen) {
        msgproc_state = awaiting_header_byte0;
      }
      else {
        msgproc_substripe_index = 0;
      }
    }
    break;
  case chomping_msg:
    if (msgproc_msgcnt >= msgproc_msglen) {
      msgproc_state = awaiting_header_byte0;
    }
    else {
      msgproc_msgcnt++;
    }
    break;
  }
}

struct stripe_type msgproc_convert_device_stripe(device_stripe_type ds)
{
  struct stripe_type returnval;
  uint8_t w;
  bool vline_0x100_bit;
  bool hstart_0x100_bit;
  bool hstart_0x200_bit;
  bool hstop_0x100_bit;
  bool hstop_0x200_bit;

  returnval.vline = (uint16_t) ds[0];
  returnval.hstart = (uint16_t) ds[1];
  returnval.hstop = (uint16_t) ds[2];
  w = ds[3];

  vline_0x100_bit = ((w & 0x20) != 0);
  if (vline_0x100_bit)
    returnval.vline += 0x100;

  hstart_0x100_bit = ((w & 0x80) != 0);
  hstart_0x200_bit = ((w & 0x10) != 0);
  if (hstart_0x200_bit)
    returnval.hstart += 0x200;
  if (hstart_0x100_bit)
    returnval.hstart += 0x100;

  hstop_0x100_bit = ((w & 0x40) != 0);
  hstop_0x200_bit = ((w & 0x08) != 0);
  if (hstop_0x200_bit)
    returnval.hstop += 0x200;
  if (hstop_0x100_bit)
    returnval.hstop += 0x100;

  return returnval;
}

/* stripes must be added in vline sorted order!
 * this is true for the tir4 hardware */
void msgproc_add_stripe(struct stripe_type s)
{
  /* protobloblist iterator (node) */
  struct protobloblist_iter *bli, *nextbli;
  /* a working blob structure */
  struct protoblob_type wb;
  bool stripe_match_found = false;
  struct protobloblist_iter *first_stripe_match_position;

  /* if we are past the lines of any open blob,
   * close it off */
  bli = protobloblist_get_iter(&msgproc_open_blobs);
  while (!protobloblist_iter_complete(bli)) {
    nextbli = protobloblist_iter_next(bli);
    if (bli->pblob.stripes.head->stripe.vline < s.vline - 2) {
      protobloblist_add_protoblob(&msgproc_closed_blobs,
                                  bli->pblob);
      protobloblist_delete(&msgproc_open_blobs,
                           bli);
    }
    bli = nextbli;
  }

  /* any open blob that contacts this stripe gets merged */
  bli = protobloblist_get_iter(&msgproc_open_blobs);
  while (!protobloblist_iter_complete(bli)) {
    nextbli = protobloblist_iter_next(bli);
    if (protoblob_is_contact(bli->pblob, s)) {
      if (stripe_match_found == false) {
        stripe_match_found = true;
        first_stripe_match_position = bli;
      }
      else {
        protoblob_merge(&(first_stripe_match_position->pblob),
                        &(bli->pblob));
        protobloblist_delete(&msgproc_open_blobs,
                             bli);
      }
    }
    bli = nextbli;
  }

  if (!stripe_match_found) {
    protoblob_init(&wb);
    protoblob_addstripe(&wb,s);
    protobloblist_add_protoblob(&msgproc_open_blobs,
                                wb);
  }
  else {
    protoblob_addstripe(&(first_stripe_match_position->pblob),s);
  }
}

void msgproc_close_all_open_blobs(void)
{
  struct protobloblist_iter *bli, *nextbli;
  struct protoblob_type wb;

  bli = protobloblist_get_iter(&msgproc_open_blobs);
  while (!protobloblist_iter_complete(bli)) {
    nextbli = protobloblist_iter_next(bli);
    wb = bli->pblob;
    protobloblist_add_protoblob(&msgproc_closed_blobs,
                                wb);
    protobloblist_delete(&msgproc_open_blobs,
                         bli);
    bli = nextbli;
  }
}

uint16_t pixel_crop(uint16_t inpixel,
                    uint16_t offset,
                    uint16_t limitpixel)
{
  uint16_t outpixel;

  if (inpixel<offset) {
    outpixel = 0;
  }
  else {
    outpixel = inpixel - offset;
    if (outpixel > limitpixel) {
      outpixel = limitpixel;
    }
  }
  return outpixel;
}

/* tests if this stripe overlaps the argument  */
/*   in the h-axis.  Vertical overlap not tested! */
/* note: overlap is true if they share a single common  */
/* pixel */
bool stripe_is_hcontact(struct stripe_type s0,
                        struct stripe_type s1)
{
  if (s0.hstop < s1.hstart) {
    return false;
  }
  else if (s0.hstart > s1.hstop) {
    return false;
  }
  return true;
}

/* 8bpp only!
 * (resx,resy) = top right corner
 * (0,0) = bottom left corner
 */
void stripe_draw(struct stripe_type s, char *bitmap)
{
  char *startpoint=NULL;
  char *endpoint=NULL;
  uint16_t vline=0, hstart=0, hstop=0;
  /* have to crop, deinterlace and flip here */
  vline = pixel_crop(s.vline,
                     vline_offset,
                     TIR4_VERTICAL_SQUASHED_RESOLUTION-1);
  vline = TIR4_VERTICAL_RESOLUTION-2*vline-1;
  hstart = pixel_crop(s.hstart,
                      hpix_offset,
                      TIR4_HORIZONTAL_RESOLUTION-1);
  hstop = pixel_crop(s.hstop,
                     hpix_offset,
                     TIR4_HORIZONTAL_RESOLUTION-1);

  startpoint = bitmap+vline*(TIR4_HORIZONTAL_RESOLUTION) + hstart;
  endpoint = bitmap+vline*(TIR4_HORIZONTAL_RESOLUTION) + hstop;

  if (endpoint-startpoint >= 1) {
    memset(startpoint, '\255', endpoint-startpoint);
  }
}

void stripe_print(struct stripe_type s)
{
  printf("(%03d, %04d, %04d)\n",
         s.vline,
         s.hstart,
         s.hstop);
}

float point_crop(float inpoint,
                 float offset,
                 float limitpoint)
{
  float outpoint;

  if (inpoint<offset) {
    outpoint = 0;
  }
  else {
    outpoint = inpoint - offset;
    if (outpoint > limitpoint) {
      outpoint = limitpoint;
    }
  }
  return outpoint;
}

void stripelist_init(struct stripelist_type *sl)
{
  assert(sl);
  sl->head = NULL;
  sl->tail = NULL;
}

void stripelist_pop(struct stripelist_type *sl,
                    struct stripelist_iter *sli,
                    struct stripe_type *s)
{
  assert(sl);
  assert(sli);
  if (sli->prev == NULL) {
    sl->head = sli->next;
  }
  else {
    sli->prev->next = sli->next;
  }
  if (sli->next == NULL) {
    sl->tail = sli->prev;
  }
  else {
    sli->next->prev = sli->prev;
  }
  *s = sli->stripe;
  free(sli);
}

void stripelist_delete(struct stripelist_type *sl,
                       struct stripelist_iter *sli)
{
  assert(sl);
  assert(sli);
  if (sli->prev == NULL) {
    sl->head = sli->next;
  }
  else {
    sli->prev->next = sli->next;
  }
  if (sli->next == NULL) {
    sl->tail = sli->prev;
  }
  else {
    sli->next->prev = sli->prev;
  }
  free(sli);
}

struct stripelist_iter *stripelist_get_iter(struct stripelist_type *sl)
{
  assert(sl);
  return sl->head;
}

struct stripelist_iter *stripelist_iter_next(struct stripelist_iter *sli)
{
  assert(sli);
  return sli->next;
}

bool stripelist_iter_complete(struct stripelist_iter *sli)
{
  return (sli == NULL);
}

void stripelist_free(struct stripelist_type *sl)
{
  struct stripelist_iter *sli, *nextsli;

  sli = stripelist_get_iter(sl);
  while (!stripelist_iter_complete(sli)) {
    nextsli = stripelist_iter_next(sli);
    free(sli);
    sli = nextsli;
  }
}

void stripelist_push_stripe(struct stripelist_type *sl,
                           struct stripe_type s)
{
  struct stripelist_iter *newhead;
  newhead=(struct stripelist_iter *)malloc(sizeof(struct stripelist_iter));
  assert(newhead);

  newhead->stripe = s;
  newhead->prev = NULL;
  newhead->next = sl->head;
  if (sl->tail == NULL) {
    sl->tail = newhead;
  }
  if (sl->head != NULL) {
    sl->head->prev = newhead;
  }
  sl->head = newhead;
}

void stripelist_append_stripe(struct stripelist_type *sl,
                              struct stripe_type s)
{
  struct stripelist_iter *newtail;
  newtail=(struct stripelist_iter *)malloc(sizeof(struct stripelist_iter));
  assert(newtail);

  newtail->stripe = s;
  newtail->prev = sl->tail;
  newtail->next = NULL;
  if (sl->head == NULL) {
    sl->head = newtail;
  }
  if (sl->tail != NULL) {
    sl->tail->next = newtail;
  }
  sl->tail = newtail;
}

/* stripes must be added in vline sorted order! */
bool stripelist_is_contact(struct stripelist_type *sl,
                           struct stripe_type s)
{
  struct stripelist_iter *sli;

  sli = stripelist_get_iter(sl);
  while (!stripelist_iter_complete(sli)) {
    if (sli->stripe.vline < s.vline-2) {
      return false;
    }
    else if (stripe_is_hcontact(sli->stripe,s)) {
      return true;
    }
    sli = stripelist_iter_next(sli);
  }
  return false;
}

bool stripelist_is_empty(struct stripelist_type *sl)
{
  return !stripelist_iter_complete(stripelist_get_iter(sl));
}

/* sorted merge.  arguments get freed!! */
struct stripelist_type stripelist_merge(struct stripelist_type *sl0,
                                        struct stripelist_type *sl1)
{
  bool done;
  struct stripelist_iter *sli0, *sli1, *next_sli0, *next_sli1;
  struct stripelist_type result;

  stripelist_init(&result);
  sli0 = stripelist_get_iter(sl0);
  sli1 = stripelist_get_iter(sl1);

  done = false;
  while (!done) {
    if (stripelist_iter_complete(sli0) &&
        stripelist_iter_complete(sli1)) {
      done = true;
    }
    else if (stripelist_iter_complete(sli0)){
      stripelist_append_stripe(&result, sli1->stripe);
      next_sli1 = stripelist_iter_next(sli1);
      stripelist_delete(sl1,sli1);
      sli1 = next_sli1;
    }
    else if (stripelist_iter_complete(sli1)){
      stripelist_append_stripe(&result, sli0->stripe);
      next_sli0 = stripelist_iter_next(sli0);
      stripelist_delete(sl0,sli0);
      sli0 = next_sli0;
    }
    else {
      if (sli0->stripe.vline < sli1->stripe.vline) {
        stripelist_append_stripe(&result, sli0->stripe);
        next_sli0 = stripelist_iter_next(sli0);
        stripelist_delete(sl0,sli0);
        sli0 = next_sli0;
      }
      else {
        stripelist_append_stripe(&result, sli1->stripe);
        next_sli1 = stripelist_iter_next(sli1);
        stripelist_delete(sl1,sli1);
        sli1 = next_sli1;
      }
    }
  }
  return result;
}

bool stripelist_draw(struct stripelist_type *sl,
                     char *bmp)
{
  struct stripelist_iter *sli;

  sli = stripelist_get_iter(sl);
  while (!stripelist_iter_complete(sli)) {
    stripe_draw(sli->stripe,bmp);
    sli = stripelist_iter_next(sli);
  }
  return true;
}

void stripelist_print(struct stripelist_type sl)
{
  struct stripelist_iter *sli;

  printf("-- start stripelist --\n");
  sli = stripelist_get_iter(&sl);
  while (!stripelist_iter_complete(sli)) {
    stripe_print(sli->stripe);
    sli = stripelist_iter_next(sli);
  }
  printf("-- end stripelist --\n");
}

void protoblob_init(struct protoblob_type *pb)
{
  pb->cumulative_area = 0;
  pb->cumulative_2x_hcenter_area_product = 0;
  pb->cumulative_linenum_area_product = 0;
  stripelist_init(&(pb->stripes));
}

void protoblob_addstripe(struct protoblob_type *pb,
                         struct stripe_type s)
{
  int area = (s.hstop-s.hstart);

  pb->cumulative_area += area;
  pb->cumulative_linenum_area_product += s.vline*(area);
  pb->cumulative_2x_hcenter_area_product += area *
    (s.hstop+s.hstart);
  stripelist_push_stripe(&(pb->stripes), s);
}

void protoblob_merge(struct protoblob_type *pb0,
                     struct protoblob_type *pb1)
{
  pb0->cumulative_area += pb1->cumulative_area;
  pb0->cumulative_linenum_area_product +=
    pb1->cumulative_linenum_area_product;
  pb0->cumulative_2x_hcenter_area_product +=
    pb1->cumulative_2x_hcenter_area_product;
  pb0->stripes = stripelist_merge(&(pb0->stripes),
                                  &(pb1->stripes));
}

bool protoblob_is_contact(struct protoblob_type pb,
                          struct stripe_type s)
{
  return stripelist_is_contact(&(pb.stripes),s);
}

void protoblob_make_blob(struct protoblob_type *pb,
                         struct blob_type *b)
{
  float  working_x, working_y;
  /* calculate the raw values */
  working_x =((float)pb->cumulative_2x_hcenter_area_product) /
    (pb->cumulative_area * 2.0);
  working_y = ((float)pb->cumulative_linenum_area_product) / 
    pb->cumulative_area;
  /* crop them to the squashed frame size) */
  working_x = point_crop(working_x, 
                         (float) hpix_offset, 
                         TIR4_HORIZONTAL_RESOLUTION-1);
  working_y = point_crop(working_y, 
                         (float) vline_offset, 
                         TIR4_VERTICAL_SQUASHED_RESOLUTION-1);
  /* move them to 0,0 = center based coordinates */
  working_x -= (float) (TIR4_HORIZONTAL_RESOLUTION-1) / 2.0;
  working_y -= (float) (TIR4_VERTICAL_SQUASHED_RESOLUTION-1) / 2.0;
  /* need to invert the x,y */
  working_x = -working_x;
  working_y = -working_y;
  /* need to double the y */
  working_y = 2*working_y;
  b->x = working_x;
  b->y = working_y;
  b->score = pb->cumulative_area;
}

void protoblob_print(struct protoblob_type pb)
{
  printf("-- start blob --\n");
  printf("cumulative_area: %d\n", pb.cumulative_area);
  printf("cumulative_2x_hcenter_area_product: %d\n", pb.cumulative_2x_hcenter_area_product);
  printf("cumulative_linenum_area_product: %d\n", pb.cumulative_linenum_area_product);
  stripelist_print(pb.stripes);
  printf("-- end blob --\n");
}

void protobloblist_init(struct protobloblist_type *pbl)
{
  pbl->length = 0;
  pbl->head = NULL;
  pbl->tail = NULL;
}

void protobloblist_delete(struct protobloblist_type *pbl,
                          struct protobloblist_iter *pbli)
{
  pbl->length--;
  if (pbli->prev == NULL) {
    pbl->head = pbli->next;
  }
  else {
    pbli->prev->next = pbli->next;
  }
  if (pbli->next == NULL) {
    pbl->tail = pbli->prev;
  }
  else {
    pbli->next->prev = pbli->prev;
  }
  free(pbli);
}

struct protobloblist_iter *protobloblist_get_iter(struct protobloblist_type *pbl)
{
  return pbl->head;
}

struct protobloblist_iter *protobloblist_iter_next(struct protobloblist_iter *pbli)
{
  return pbli->next;
}

bool protobloblist_iter_complete(struct protobloblist_iter *pbli)
{
  return (pbli == NULL);
}

void protobloblist_free(struct protobloblist_type *pbl)
{
  struct protobloblist_iter *pbli, *nextpbli;

  pbli = protobloblist_get_iter(pbl);
  while (!protobloblist_iter_complete(pbli)) {
    nextpbli = protobloblist_iter_next(pbli);
    stripelist_free(&(pbli->pblob.stripes));
    protobloblist_delete(pbl, pbli);
    pbli = nextpbli;
  }
}

void protobloblist_add_protoblob(struct protobloblist_type *pbl,
                                 struct protoblob_type b)
{
  struct protobloblist_iter *newhead;
  newhead=(struct protobloblist_iter *)malloc(sizeof(struct protobloblist_iter));
  assert(newhead);
  newhead->pblob = b;
  newhead->prev = NULL;
  newhead->next = pbl->head;
  if (pbl->tail == NULL) {
    pbl->tail = newhead;
  }
  if (pbl->head != NULL) {
    pbl->head->prev = newhead;
  }
  pbl->head = newhead;
  pbl->length++;
}

void protobloblist_pop_biggest_protoblob(struct protobloblist_type *pbl,
                                         struct protoblob_type *pb)
{
  struct protobloblist_iter *biggest_seen_blob_iter = NULL;
  struct protobloblist_iter *pbli;

  pbli = protobloblist_get_iter(pbl);
  assert(pbli);
  while (!protobloblist_iter_complete(pbli)) {
    if (biggest_seen_blob_iter == NULL) {
      biggest_seen_blob_iter = pbli;
    }
    else if (pbli->pblob.cumulative_area >
             biggest_seen_blob_iter->pblob.cumulative_area) {
      biggest_seen_blob_iter = pbli;
    }
    pbli = protobloblist_iter_next(pbli);
  }
    *pb = biggest_seen_blob_iter->pblob;
    protobloblist_delete(pbl, biggest_seen_blob_iter);
}

bool protobloblist_populate_frame(struct protobloblist_type *pbl,
                                  struct frame_type *f,
                                  enum cal_operating_mode opmode)
{
//  struct protobloblist_iter *pbli;
  int i;
  unsigned int required_blobnum = 0;
  struct protoblob_type wb;

  if (pbl->length == 0) {
    return false;
  }

  switch (opmode) {
  case diagnostic: /* report all blobs, if there are any */
    required_blobnum = pbl->length;
    f->bitmap = (char *) malloc(BITMAP_NUM_BYTES*sizeof(char));
    assert(f->bitmap);
    memset(f->bitmap, '\0', BITMAP_NUM_BYTES);
    protobloblist_draw(pbl,
                       f->bitmap);
    break;
  case operational_1dot:
    required_blobnum = 1;
    break;
  case operational_3dot:
    required_blobnum = 3;
    break;
  }
  if (pbl->length < required_blobnum) {
    return false;
  }

  f->bloblist.num_blobs = required_blobnum;
  f->bloblist.blobs = (struct blob_type *)
    malloc(f->bloblist.num_blobs*sizeof(struct blob_type));
  assert(f->bloblist.blobs);

  for(i=0; i<required_blobnum;i++) {
    protobloblist_pop_biggest_protoblob(pbl, &wb);
    protoblob_make_blob(&wb,
                        &(f->bloblist.blobs[i]));
    stripelist_free(&(wb.stripes));
    
  }
  return true;
}

void protobloblist_draw(struct protobloblist_type *pbl,
                        char *bmp)
{
  struct protobloblist_iter *pbli;

  pbli = protobloblist_get_iter(pbl);
  while (!protobloblist_iter_complete(pbli)) {
    stripelist_draw(&(pbli->pblob.stripes), bmp);
    pbli = protobloblist_iter_next(pbli);
  }
}

void protobloblist_print(struct protobloblist_type pbl)
{
  struct protobloblist_iter *pbli;

  printf("-- start frame --\n");
  pbli = protobloblist_get_iter(&pbl);
  while (!protobloblist_iter_complete(pbli)) {
    protoblob_print(pbli->pblob);
    pbli = protobloblist_iter_next(pbli);
  }
  printf("-- end frame --\n");
}

void framelist_init(struct framelist_type *fl)
{
  fl->head = NULL;
  fl->tail = NULL;
}

void framelist_pop(struct framelist_type *fl,
                   struct framelist_iter *fli,
                   struct frame_type *f)
{
  if (fli->prev == NULL) {
    fl->head = fli->next;
  }
  else {
    fli->prev->next = fli->next;
  }
  if (fli->next == NULL) {
    fl->tail = fli->prev;
  }
  else {
    fli->next->prev = fli->prev;
  }
  *f = fli->frame;
  free(fli);
}

struct framelist_iter *framelist_get_iter(struct framelist_type *fl)
{
  return fl->head;
}

struct framelist_iter *framelist_iter_next(struct framelist_iter *fli)
{
  return fli->next;
}

bool framelist_iter_complete(struct framelist_iter *fli)
{
  return (fli == NULL);
}

void framelist_flush(struct camera_control_block *ccb,
                     struct framelist_type *fl)
{
  struct framelist_iter *fli, *nextfli;
  struct frame_type *f;
  fli = framelist_get_iter(fl);
  while (!framelist_iter_complete(fli)) {
    nextfli = framelist_iter_next(fli);
    framelist_pop(fl, fli,f);
    frame_free(ccb, f);
    fli = nextfli;
  }
}

void framelist_add_frame(struct framelist_type *fl,
                         struct frame_type f)
{
  struct framelist_iter *newhead;
  newhead=(struct framelist_iter *)malloc(sizeof(struct framelist_iter));
  assert(newhead);

  newhead->frame = f;
  newhead->prev = NULL;
  newhead->next = fl->head;
  if (fl->tail == NULL) {
    fl->tail = newhead;
  }
  if (fl->head != NULL) {
    fl->head->prev = newhead;
  }
  fl->head = newhead;
}

void framelist_print(struct framelist_type *fl)
{
  struct framelist_iter *fli;

  printf("-- start frame --\n");
  fli = framelist_get_iter(fl);
  while (!framelist_iter_complete(fli)) {
    frame_print(fli->frame);
    fli = framelist_iter_next(fli);
  }
  printf("-- end frame --\n");
}

bool tir4_is_frame_available(void)
{
  return !framelist_iter_complete(framelist_get_iter(&master_framelist));
}
