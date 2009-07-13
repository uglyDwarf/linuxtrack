#include <stdio.h>
#include <stdlib.h> /* for malloc */
#include <stdint.h>
#include "tir4_driver.h"

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
#define BULK_READ_SIZE 0X4000
#define BULK_READ_TIMEOUT 4000   /* in milliseconds */
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


/**********************/
/* private data types */
/**********************/
struct stripe_type {
  uint16_t vline;
  uint16_t hstart;
  uint16_t hstop;
};

struct stripestack_type {
  struct stripe_type stripe;
  struct stripestack_type *next;
  struct stripestack_type *prev;
};

struct protoblob_type {
  /* total # pixels area, used for sorting/scoring blobs */
  unsigned int cumulative_area;
  /* used to calculate x */
  unsigned int cumulative_2x_hcenter_area_product;
  /* used to calculate y */
  unsigned int cumulative_linenum_area_product;
  /* used to identify blobs */
  struct stripestack_type *stripes;
};

struct protobloblist_type {
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
  struct tir4_frame_type frame;
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
/* static bool crop_frames = true; */
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

/*******************************/
/* private function prototypes */
/*******************************/
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
void msgproc_init(void);
void msgproc_add_byte(uint8_t b);
struct stripe_type msgproc_convert_device_stripe(device_stripe_type ds);
void msgproc_add_stripe(struct stripe_type s);
void msgproc_close_all_open_blobs(void);
bool stripe_is_hcontact(struct stripe_type s0,
                        struct stripe_type s1);
void stripe_print(struct stripe_type s);
struct stripestack_type *stripestack_new(void);
void stripestack_free(struct stripestack_type *s_stk);
struct stripestack_type *stripestack_push(struct stripestack_type *s_stk,
                                          struct stripe_type s);
struct stripestack_type *stripestack_next(struct stripestack_type *s_stk);
bool stripestack_is_empty(struct stripestack_type *s_stk);
struct stripestack_type *stripestack_merge(struct stripestack_type *s_stk0,
                                           struct stripestack_type *s_stk1);
bool stripestack_is_contact(struct stripestack_type *s_stk,
                            struct stripe_type s);
void protoblob_init(struct protoblob_type *pb);
void protoblob_addstripe(struct protoblob_type *pb,
                         struct stripe_type s);
void protoblob_merge(struct protoblob_type *pb0,
                     struct protoblob_type *pb1);
bool protoblob_is_contact(struct protoblob_type pb,
                          struct stripe_type s);
void protoblob_make_tir4_blob(struct protoblob_type *pb,
                              struct tir4_blob_type *t4b);
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
void protobloblist_get_tir4_frame(struct protobloblist_type *pbl,
                                  struct tir4_frame_type *f);
void protobloblist_print(struct protobloblist_type pbl);
/* FIXME: add bloblist sort!! */
/* struct protobloblist_type *protobloblist_sort(struct protobloblist_type *pbl); */
void framelist_init(struct framelist_type *fl);
void framelist_delete(struct framelist_type *fl,
                      struct framelist_iter *fli);
struct framelist_iter *framelist_get_iter(struct framelist_type *fl);
struct framelist_iter *framelist_iter_next(struct framelist_iter *fli);
bool framelist_iter_complete(struct framelist_iter *fli);
void framelist_free(struct framelist_type *fl);
void framelist_add_tir4_frame(struct framelist_type *fl,
                              struct tir4_frame_type t4f);
void framelist_print(struct framelist_type *fl);


/************************/
/* function definitions */
/************************/
void tir4_init()
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
    tir4_error_alert("Unable to locate TIR4 device\n");
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

  msgproc_init();
  framelist_init(&master_framelist);
  
/*   int i; */
/*   for(i=0; i<49; i++) { */
/*     stripe_print(testblob[i]); */
/*     msgproc_add_stripe(testblob[i]); */
/*   } */
/*   msgproc_close_all_open_blobs(); */
/*   protobloblist_print(&msgproc_closed_blobs); */
/*   protobloblist_free(&msgproc_closed_blobs); */
/*   exit(1); */
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

void tir4_release(void)
{
  int ret;
  ret = usb_release_interface(tir4_handle, 0);
  if (ret < 0) {
    tir4_error_alert("failed to release TIR4 interface.\n");
    exit(1);
  }
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

void tir4_error_alert(char *message)
{
  fprintf(stderr, message);
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
      tir4_error_alert("Invalid state while reading bulk config datafile.\n");
      tir4_shutdown();
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
}

void msgproc_add_byte(uint8_t b)
{
  int i;
  bool is_vsync;
  struct tir4_frame_type t4f;

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
      fprintf(stderr,"Warning USB UNKNOWN DATA: 0x%02x\n", msgproc_msglen);
      msgproc_msglen = b;
      msgproc_state = awaiting_header_byte1;
    }
    break;
  case processing_msg:
    /* first 3 bytes of the device stripe */
    if (msgproc_substripe_index < (DEVICE_STRIPE_LEN-1)) {
      msgproc_msgcnt++;
      if (msgproc_msgcnt >= msgproc_msglen) {
        fprintf(stderr,"Warning USB packet ended midstripe. Packet dropped.\n");
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
        msgproc_add_stripe(newstripe); 
      }
      else {
        /* about done, just move any open to closed blobs */
        msgproc_close_all_open_blobs();
        protobloblist_get_tir4_frame(&msgproc_closed_blobs,
                                     &t4f);
        if (t4f.num_blobs > 0) {
          framelist_add_tir4_frame(&master_framelist, t4f);
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
    wb = bli->pblob;
    if (wb.stripes->stripe.vline < s.vline - 2) {
      protobloblist_add_protoblob(&msgproc_closed_blobs,
                                  wb);
      protobloblist_delete(&msgproc_open_blobs,
                           bli);
    }
    bli = nextbli;
  }

  /* any open blob that contacts this stripe gets merged */
  bli = protobloblist_get_iter(&msgproc_open_blobs);
  while (!protobloblist_iter_complete(bli)) {
    nextbli = protobloblist_iter_next(bli);
    wb = bli->pblob;
    if (protoblob_is_contact(wb, s)) {
      if (stripe_match_found == false) {
        stripe_match_found = true;
        first_stripe_match_position = bli;
      }
      else {
        protoblob_merge(&(first_stripe_match_position->pblob),
                        &(wb));
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

void stripe_print(struct stripe_type s)
{
  printf("(%03d, %04d, %04d)\n",
         s.vline,
         s.hstart,
         s.hstop);
}

struct stripestack_type *stripestack_new(void)
{
  return NULL;
}

void stripestack_free(struct stripestack_type *s_stk)
{
  struct stripestack_type *nodeptr, *nextnodeptr;

  nodeptr = s_stk;
  while (!stripestack_is_empty(nodeptr)) {
    nextnodeptr = stripestack_next(nodeptr);
    free(nodeptr);
    nodeptr = nextnodeptr;
  }
}

struct stripestack_type *stripestack_push(struct stripestack_type *s_stk,
                                          struct stripe_type s)
{
  struct stripestack_type *newhead;
  newhead=(struct stripestack_type *)malloc(sizeof(struct stripestack_type));
  newhead->stripe = s;
  newhead->next = s_stk;
  newhead->prev = NULL;
  if (!stripestack_is_empty(s_stk)) {
    s_stk->prev = newhead;
  }
  return newhead;
}

struct stripestack_type *stripestack_next(struct stripestack_type *s_stk)
{
  if (stripestack_is_empty(s_stk)) {
    tir4_error_alert("Attempt to advance stripestack to next on null.\n");
    tir4_shutdown();
    exit(1);
  }
  return s_stk->next;
}

bool stripestack_is_empty(struct stripestack_type *s_stk)
{
  return (s_stk == NULL);
}

/* sorted merge.  arguments get freed!! */
/* FIXME: this could be done all with pointers and no
 * reallocations! */
/* FIXME: when a list becomes empty, I don't need to
 * keep iterating on it; i could just point result to the remainder
 */
struct stripestack_type *stripestack_merge(struct stripestack_type *s_stk0,
                                           struct stripestack_type *s_stk1)
{
  bool done;
  struct stripestack_type *np0, *np1, *next_np0, *next_np1, *result;

  result = stripestack_new();
  np0 = s_stk0;
  np1 = s_stk1;

  done = false;
  while (!done) {
    if (stripestack_is_empty(np0) &&
        stripestack_is_empty(np1)) {
      done = true;
    }
    else if (stripestack_is_empty(np0)){
      stripestack_push(result, np1->stripe);
      next_np1 = stripestack_next(np1);
      free(np1);
      np1 = next_np1;
    }
    else if (stripestack_is_empty(np1)){
      stripestack_push(result, np0->stripe);
      next_np0 = stripestack_next(np0);
      free(np0);
      np0 = next_np0;
    }
    else {
      if (np0->stripe.vline < np1->stripe.vline) {
        stripestack_push(result, np0->stripe);
        next_np0 = stripestack_next(np0);
        free(np0);
        np0 = next_np0;
      }
      else {
        stripestack_push(result, np1->stripe);
        next_np1 = stripestack_next(np1);
        free(np1);
        np1 = next_np1;
      }
    }
  }
  return result;
}

/* stripes must be added in vline sorted order! */
bool stripestack_is_contact(struct stripestack_type *s_stk,
                            struct stripe_type s)
{
  struct stripestack_type *np;

  for (np = s_stk;
       !stripestack_is_empty(np);
       np = stripestack_next(np)) {
    if (np->stripe.vline < s.vline-2) {
      return false;
    }
    else if (stripe_is_hcontact(np->stripe,s)) {
      return true;
    }
  }
  return false;
}

void stripestack_print(struct stripestack_type *s_stk)
{
  struct stripestack_type *np;

  printf("-- start stripestack --\n");
  for (np = s_stk;
       !stripestack_is_empty(np);
       np = stripestack_next(np)) {
    stripe_print(np->stripe);
  }
  printf("-- end stripestack --\n");
}

void protoblob_init(struct protoblob_type *pb)
{
  pb->cumulative_area = 0;
  pb->cumulative_2x_hcenter_area_product = 0;
  pb->cumulative_linenum_area_product = 0;
  pb->stripes = stripestack_new();
}

void protoblob_addstripe(struct protoblob_type *pb,
                         struct stripe_type s)
{
  int area = (s.hstop-s.hstart);

  pb->cumulative_area += area;
  pb->cumulative_linenum_area_product += s.vline*(area);
  pb->cumulative_2x_hcenter_area_product += area *
    (s.hstop+s.hstart);
  pb->stripes = stripestack_push(pb->stripes, s);
}

void protoblob_merge(struct protoblob_type *pb0,
                     struct protoblob_type *pb1)
{
  pb0->cumulative_area += pb1->cumulative_area;
  pb0->cumulative_linenum_area_product +=
    pb1->cumulative_linenum_area_product;
  pb0->cumulative_2x_hcenter_area_product +=
    pb1->cumulative_2x_hcenter_area_product;
  pb0->stripes = stripestack_merge(pb0->stripes,
                                   pb1->stripes);
}

bool protoblob_is_contact(struct protoblob_type pb,
                          struct stripe_type s)
{
  return stripestack_is_contact(pb.stripes,s);
}

void protoblob_make_tir4_blob(struct protoblob_type *pb,
                              struct tir4_blob_type *t4b)
{
  t4b->x = (((float)pb->cumulative_2x_hcenter_area_product) / 
            (pb->cumulative_area * 2.0)) - hpix_offset;
  t4b->y = 2.0 * 
    (((float)pb->cumulative_linenum_area_product) / pb->cumulative_area) -
    vline_offset;
  t4b->area = pb->cumulative_area;
}

void protoblob_print(struct protoblob_type pb)
{
  printf("-- start blob --\n");
  printf("cumulative_area: %d\n", pb.cumulative_area);
  printf("cumulative_2x_hcenter_area_product: %d\n", pb.cumulative_2x_hcenter_area_product);
  printf("cumulative_linenum_area_product: %d\n", pb.cumulative_linenum_area_product);
  stripestack_print(pb.stripes);
  printf("-- end blob --\n");
}

void protobloblist_init(struct protobloblist_type *pbl)
{
  pbl->head = NULL;
  pbl->tail = NULL;
}

void protobloblist_delete(struct protobloblist_type *pbl,
                          struct protobloblist_iter *pbli)
{
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
    protobloblist_delete(pbl, pbli);
    pbli = nextpbli;
  }
}
  
void protobloblist_add_protoblob(struct protobloblist_type *pbl,
                                 struct protoblob_type b)
{
  struct protobloblist_iter *newhead;
  newhead=(struct protobloblist_iter *)malloc(sizeof(struct protobloblist_iter));

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
}

void protobloblist_get_tir4_frame(struct protobloblist_type *pbl,
                                  struct tir4_frame_type *f)
{
  struct protobloblist_iter *pbli;
  struct tir4_blob_type t4b;

  f->num_blobs=0;
  tir4_bloblist_init(&(f->bloblist));

  pbli = protobloblist_get_iter(pbl);
  while (!protobloblist_iter_complete(pbli)) {
    protoblob_make_tir4_blob(&(pbli->pblob),
                             &t4b);
    f->num_blobs++;
    tir4_bloblist_add_tir4_blob(&(f->bloblist),
                                t4b);
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

void tir4_blob_print(struct tir4_blob_type b)
{
  printf("x: %f\ty: %f\tarea: %d\n", b.x,b.y,b.area);
}

void tir4_bloblist_init(struct tir4_bloblist_type *t4bl)
{
  t4bl->head = NULL;
  t4bl->tail = NULL;
}

void tir4_bloblist_delete(struct tir4_bloblist_type *t4bl,
                          struct tir4_bloblist_iter *t4bli)
{
  if (t4bli->prev == NULL) {
    t4bl->head = t4bli->next; 
  }
  else {
    t4bli->prev->next = t4bli->next;
  }
  if (t4bli->next == NULL) {
    t4bl->tail = t4bli->prev; 
  }
  else {
    t4bli->next->prev = t4bli->prev;
  }
  free(t4bli);
}

struct tir4_bloblist_iter *tir4_bloblist_get_iter(struct tir4_bloblist_type *t4bl)
{
  return t4bl->head;
}

struct tir4_bloblist_iter *tir4_bloblist_iter_next(struct tir4_bloblist_iter *t4bli)
{
  return t4bli->next;
}

bool tir4_bloblist_iter_complete(struct tir4_bloblist_iter *t4bli)
{
  return (t4bli == NULL);
}

void tir4_bloblist_free(struct tir4_bloblist_type *t4bl)
{
  struct tir4_bloblist_iter *t4bli, *nextt4bli;

  t4bli = tir4_bloblist_get_iter(t4bl);
  while (!tir4_bloblist_iter_complete(t4bli)) {
    nextt4bli = tir4_bloblist_iter_next(t4bli);
    tir4_bloblist_delete(t4bl, t4bli);
    t4bli = nextt4bli;
  }
}
  
void tir4_bloblist_add_tir4_blob(struct tir4_bloblist_type *t4bl,
                                 struct tir4_blob_type b)
{
  struct tir4_bloblist_iter *newhead;
  newhead=(struct tir4_bloblist_iter *)malloc(sizeof(struct tir4_bloblist_iter));

  newhead->blob = b;
  newhead->prev = NULL;
  newhead->next = t4bl->head;
  if (t4bl->tail == NULL) {
    t4bl->tail = newhead;
  }
  if (t4bl->head != NULL) {
    t4bl->head->prev = newhead;
  }
  t4bl->head = newhead;
}

void tir4_bloblist_print(struct tir4_bloblist_type *t4bl)
{
  struct tir4_bloblist_iter *t4bli;

  printf("-- start blob --\n");
  t4bli = tir4_bloblist_get_iter(t4bl);
  while (!tir4_bloblist_iter_complete(t4bli)) {
    tir4_blob_print(t4bli->blob);
    t4bli = tir4_bloblist_iter_next(t4bli);
  }
  printf("-- end blob --\n");
}

void framelist_init(struct framelist_type *fl)
{
  fl->head = NULL;
  fl->tail = NULL;
}

void framelist_delete(struct framelist_type *fl,
                      struct framelist_iter *fli)
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

void framelist_free(struct framelist_type *fl)
{
  struct framelist_iter *fli, *nextfli;

  fli = framelist_get_iter(fl);
  while (!framelist_iter_complete(fli)) {
    nextfli = framelist_iter_next(fli);
    framelist_delete(fl, fli);
    fli = nextfli;
  }
}

void framelist_add_tir4_frame(struct framelist_type *fl,
                              struct tir4_frame_type t4f)
{
  struct framelist_iter *newhead;
  newhead=(struct framelist_iter *)malloc(sizeof(struct framelist_iter));

  newhead->frame = t4f;
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
/*     tir4_frame_print(fli->flob); */
    fli = framelist_iter_next(fli);
  }
  printf("-- end frame --\n");
}

void tir4_do_read(void)
{
  int numread;
  numread = usb_bulk_read(tir4_handle,
                      TIR_BULK_IN_EP,
                      usb_read_buf,
                      BULK_READ_SIZE,
                      BULK_READ_TIMEOUT);
  if (numread < 0) {
    tir4_error_alert("Error during TIR4 device USB BULK READ.\n");
    printf("Errno: %s\n",usb_strerror());
    /* FIXME: timeouts read as errors here!! */
/*     if (numread == LIBUSB_SUCCESS) { */
/*       printf("I think it was a timeout\n"); */
/*     } */
    tir4_shutdown();
    exit(1);
  }
  int i;

  for (i=0; i<numread; i++) {
    msgproc_add_byte((uint8_t) usb_read_buf[i]);
  }
}

bool tir4_frame_is_available(void)
{
  return !framelist_iter_complete(framelist_get_iter(&master_framelist));
}

void tir4_get_frame(struct tir4_frame_type *f)
{
  f->bloblist = (master_framelist.tail)->frame.bloblist;
  f->num_blobs = master_framelist.tail->frame.num_blobs;
  framelist_delete(&master_framelist,master_framelist.tail);
}

void tir4_frame_print(struct tir4_frame_type *f)
{
  printf("-- start frame --\n");
  printf("num blobs: %d\n", f->num_blobs);
  tir4_bloblist_print(&(f->bloblist));
  printf("-- end frame --\n");
}
