#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include "utils.h"
#define USB_IMPL_ONLY
#include "usb_ifc.h"
#include <math.h>
#include <string.h>
#include "ps3_prefs.h"
#include "cal.h"
#ifndef OPENCV
#include "image_process.h"
#else
#include "facetrack.h"
#endif


#define OV534_REG_ADDRESS       0xf1    /* sensor address */
#define OV534_REG_SUBADDR       0xf2
#define OV534_REG_WRITE         0xf3
#define OV534_REG_READ          0xf4
#define OV534_REG_OPERATION     0xf5
#define OV534_REG_STATUS        0xf6

#define OV534_OP_WRITE_3        0x37
#define OV534_OP_WRITE_2        0x33
#define OV534_OP_READ_2         0xf9

#define CTRL_TIMEOUT 500

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define V4L2_EXPOSURE_AUTO   1
#define V4L2_EXPOSURE_MANUAL 0




static const uint8_t qvga_rates[] = {187, 150, 137, 125, 100, 75, 60, 50, 37, 30};
static const uint8_t vga_rates[] = {60, 50, 40, 30, 15};

struct framerates{
  const uint8_t *rates;
  size_t         nrates;
};

static const struct framerates ov772x_framerates[] = {
        { /* 320x240 */
                .rates = qvga_rates,
                .nrates = ARRAY_SIZE(qvga_rates),
        },
        { /* 640x480 */
                .rates = vga_rates,
                .nrates = ARRAY_SIZE(vga_rates),
        },
};

static const uint8_t bridge_init_772x[][2] = {
        { 0xc2, 0x0c },
        { 0x88, 0xf8 },
        { 0xc3, 0x69 },
        { 0x89, 0xff },
        { 0x76, 0x03 },
        { 0x92, 0x01 },
        { 0x93, 0x18 },
        { 0x94, 0x10 },
        { 0x95, 0x10 },
        { 0xe2, 0x00 },
        { 0xe7, 0x3e },

        { 0x96, 0x00 },

        { 0x97, 0x20 },
        { 0x97, 0x20 },
        { 0x97, 0x20 },
        { 0x97, 0x0a },
        { 0x97, 0x3f },
        { 0x97, 0x4a },
        { 0x97, 0x20 },
        { 0x97, 0x15 },
        { 0x97, 0x0b },

        { 0x8e, 0x40 },
        { 0x1f, 0x81 },
        { 0x34, 0x05 },
        { 0xe3, 0x04 },
        { 0x88, 0x00 },
        { 0x89, 0x00 },
        { 0x76, 0x00 },
        { 0xe7, 0x2e },
        { 0x31, 0xf9 },
        { 0x25, 0x42 },
        { 0x21, 0xf0 },

        { 0x1c, 0x00 },
        { 0x1d, 0x40 },
        { 0x1d, 0x02 }, /* payload size 0x0200 * 4 = 2048 bytes */
        { 0x1d, 0x00 }, /* payload size */

        { 0x1d, 0x02 }, /* frame size 0x025800 * 4 = 614400 */
        { 0x1d, 0x58 }, /* frame size */
        { 0x1d, 0x00 }, /* frame size */

        { 0x1c, 0x0a },
        { 0x1d, 0x08 }, /* turn on UVC header */
        { 0x1d, 0x0e }, /* .. */

        { 0x8d, 0x1c },
        { 0x8e, 0x80 },
        { 0xe5, 0x04 },

        { 0xc0, 0x50 },
        { 0xc1, 0x3c },
        { 0xc2, 0x0c },
};
static const uint8_t sensor_init_772x[][2] = {
        { 0x12, 0x80 },
        { 0x11, 0x01 },
/*fixme: better have a delay?*/
        { 0x11, 0x01 },
        { 0x11, 0x01 },
        { 0x11, 0x01 },
        { 0x11, 0x01 },
        { 0x11, 0x01 },
        { 0x11, 0x01 },
        { 0x11, 0x01 },
        { 0x11, 0x01 },
        { 0x11, 0x01 },
        { 0x11, 0x01 },

        { 0x3d, 0x03 },
        { 0x17, 0x26 },
        { 0x18, 0xa0 },
        { 0x19, 0x07 },
        { 0x1a, 0xf0 },
        { 0x32, 0x00 },
        { 0x29, 0xa0 },
        { 0x2c, 0xf0 },
        { 0x65, 0x20 },
        { 0x11, 0x01 },
        { 0x42, 0x7f },
        { 0x63, 0xaa },         /* AWB - was e0 */
        { 0x64, 0xff },
        { 0x66, 0x00 },
        { 0x13, 0xf0 },         /* com8 */
        { 0x0d, 0x41 },
        { 0x0f, 0xc5 },
        { 0x14, 0x11 },

        { 0x22, 0x7f },
        { 0x23, 0x03 },
        { 0x24, 0x40 },
        { 0x25, 0x30 },
        { 0x26, 0xa1 },
        { 0x2a, 0x00 },
        { 0x2b, 0x00 },
        { 0x6b, 0xaa },
        { 0x13, 0xff },         /* AWB */

        { 0x90, 0x05 },
        { 0x91, 0x01 },
        { 0x92, 0x03 },
        { 0x93, 0x00 },
        { 0x94, 0x60 },
        { 0x95, 0x3c },
        { 0x96, 0x24 },
        { 0x97, 0x1e },
        { 0x98, 0x62 },
        { 0x99, 0x80 },
        { 0x9a, 0x1e },
        { 0x9b, 0x08 },
        { 0x9c, 0x20 },
        { 0x9e, 0x81 },

        { 0xa6, 0x07 },
        { 0x7e, 0x0c },
        { 0x7f, 0x16 },
        { 0x80, 0x2a },
        { 0x81, 0x4e },
        { 0x82, 0x61 },
        { 0x83, 0x6f },
        { 0x84, 0x7b },
        { 0x85, 0x86 },
        { 0x86, 0x8e },
        { 0x87, 0x97 },
        { 0x88, 0xa4 },
        { 0x89, 0xaf },
        { 0x8a, 0xc5 },
        { 0x8b, 0xd7 },
        { 0x8c, 0xe8 },
        { 0x8d, 0x20 },

        { 0x0c, 0x90 },

        { 0x2b, 0x00 },
        { 0x22, 0x7f },
        { 0x23, 0x03 },
        { 0x11, 0x01 },
        { 0x0c, 0xd0 },
        { 0x64, 0xff },
        { 0x0d, 0x41 },

        { 0x14, 0x41 },
        { 0x0e, 0xcd },
        { 0xac, 0xbf },
        { 0x8e, 0x00 },         /* De-noise threshold */
        { 0x0c, 0xd0 }
};
static const uint8_t bridge_start_vga_772x[][2] = {
        {0x1c, 0x00},
        {0x1d, 0x40},
        {0x1d, 0x02},
        {0x1d, 0x00},
        {0x1d, 0x02},
        {0x1d, 0x58},
        {0x1d, 0x00},
        {0xc0, 0x50},
        {0xc1, 0x3c},
};
static const uint8_t sensor_start_vga_772x[][2] = {
        {0x12, 0x00},
        {0x17, 0x26},
        {0x18, 0xa0},
        {0x19, 0x07},
        {0x1a, 0xf0},
        {0x29, 0xa0},
        {0x2c, 0xf0},
        {0x65, 0x20},
};
static const uint8_t bridge_start_qvga_772x[][2] = {
        {0x1c, 0x00},
        {0x1d, 0x40},
        {0x1d, 0x02},
        {0x1d, 0x00},
        {0x1d, 0x01},
        {0x1d, 0x4b},
        {0x1d, 0x00},
        {0xc0, 0x28},
        {0xc1, 0x1e},
};
static const uint8_t sensor_start_qvga_772x[][2] = {
        {0x12, 0x40},
        {0x17, 0x3f},
        {0x18, 0x50},
        {0x19, 0x03},
        {0x1a, 0x78},
        {0x29, 0x50},
        {0x2c, 0x78},
        {0x65, 0x2f},
};

static int usb_err = 0;

static void ov534_reg_write(uint16_t reg, uint8_t val)
{
  bool ret;
  if(usb_err < 0){
    return;
  }

  ltr_int_log_message( "SET 01 0000 %04x %02x\n", reg, val);
  ret = ltr_int_ctrl_data(
          /*LIBUSB_ENDPOINT_OUT*/ (0x00) |
          /*LIBUSB_REQUEST_TYPE_VENDOR*/ (0x02 << 5) |
          /*LIBUSB_RECIPIENT_DEVICE*/ (0x00),
          0x01, 0x00, reg, &val, 1);
  if(!ret){
    ltr_int_log_message("ov534_reg_write failed\n");
    usb_err = -1;
  }
}

static uint8_t ov534_reg_read(uint16_t reg)
{
  int ret;
  uint8_t data;

  if(usb_err < 0){
    return 0;
  }
  ret = ltr_int_ctrl_data(
          /*LIBUSB_ENDPOINT_IN*/ (0x80) |
          /*LIBUSB_REQUEST_TYPE_VENDOR*/ (0x02 << 5) |
          /*LIBUSB_RECIPIENT_DEVICE*/ (0x00),
          0x01, 0x00, reg, &data, 1);
  ltr_int_log_message("GET 01 0000 %04x %02x\n", reg, data);
  if(!ret){
    ltr_int_log_message("ov534_reg_read failed\n");
    usb_err = -1;
  }
  return data;
}

// * Two bits control LED: 0x21 bit 7 and 0x23 bit 7.
// * (direction and output)?
static void ov534_set_led(int status)
{
  uint8_t data;

  ltr_int_log_message("led status: %d\n", status);

  data = ov534_reg_read(0x21);
  data |= 0x80;
  ov534_reg_write(0x21, data);

  data = ov534_reg_read(0x23);
  if(status){
    data |= 0x80;
  }else{
    data &= ~0x80;
  }
  ov534_reg_write(0x23, data);

  if(!status){
    data = ov534_reg_read(0x21);
    data &= ~0x80;
    ov534_reg_write(0x21, data);
  }
}

static int sccb_check_status()
{
  uint8_t data;
  int i;

  for(i = 0; i < 5; i++){
    usleep(10000);
    data = ov534_reg_read(OV534_REG_STATUS);

    switch(data){
      case 0x00:
        return 1;
        break;
      case 0x04:
        return 0;
        break;
      case 0x03:
        break;
      default:
        ltr_int_log_message("sccb status 0x%02x, attempt %d\n", data, i + 1);
    }
  }
  return 0;
}

static void sccb_reg_write(uint8_t reg, uint8_t val)
{
  ltr_int_log_message("sccb write: %02x %02x\n", reg, val);
  ov534_reg_write(OV534_REG_SUBADDR, reg);
  ov534_reg_write(OV534_REG_WRITE, val);
  ov534_reg_write(OV534_REG_OPERATION, OV534_OP_WRITE_3);

  if(!sccb_check_status()){
    ltr_int_log_message("sccb_reg_write failed\n");
    usb_err = -EIO;
  }
}

static uint8_t sccb_reg_read(uint16_t reg)
{
  ov534_reg_write(OV534_REG_SUBADDR, reg);
  ov534_reg_write(OV534_REG_OPERATION, OV534_OP_WRITE_2);
  if(!sccb_check_status()){
    ltr_int_log_message("sccb_reg_read failed 1\n");
  }

  ov534_reg_write(OV534_REG_OPERATION, OV534_OP_READ_2);
  if(!sccb_check_status()){
    ltr_int_log_message("sccb_reg_read failed 2\n");
  }

  return ov534_reg_read(OV534_REG_READ);
}

// output a bridge sequence (reg - val)
static void reg_w_array(const uint8_t (*data)[2], int len)
{
  while(--len >= 0){
    ov534_reg_write((*data)[0], (*data)[1]);
    data++;
  }
}

// output a sensor sequence (reg - val)
static void sccb_w_array(const uint8_t (*data)[2], int len)
{
  while (--len >= 0) {
    if ((*data)[0] != 0xff) {
      sccb_reg_write((*data)[0], (*data)[1]);
    } else {
      sccb_reg_read((*data)[1]);
      sccb_reg_write(0xff, 0x00);
    }
    data++;
  }
}



static int curr_mode = 0;//0 - 320x240, 1 - 640x480

// ov772x specific controls
static void set_frame_rate(uint8_t frame_rate)
{
  int i;
  struct rate_s {
    uint8_t fps;
    uint8_t r11;
    uint8_t r0d;
    uint8_t re5;
  };
  const struct rate_s *r;
  static const struct rate_s rate_0[] = { // 640x480
    {60, 0x01, 0xc1, 0x04},
    {50, 0x01, 0x41, 0x02},
    {40, 0x02, 0xc1, 0x04},
    {30, 0x04, 0x81, 0x02},
    {15, 0x03, 0x41, 0x04},
  };
  static const struct rate_s rate_1[] = { // 320x240
//    {205, 0x01, 0xc1, 0x02},  * 205 FPS: video is partly corrupt *
    {187, 0x01, 0x81, 0x02}, // 187 FPS or below: video is valid *
    {150, 0x01, 0xc1, 0x04},
    {137, 0x02, 0xc1, 0x02},
    {125, 0x02, 0x81, 0x02},
    {100, 0x02, 0xc1, 0x04},
    {75, 0x03, 0xc1, 0x04},
    {60, 0x04, 0xc1, 0x04},
    {50, 0x02, 0x41, 0x04},
    {37, 0x03, 0x41, 0x04},
    {30, 0x04, 0x41, 0x04},
  };

  if(curr_mode == 1){
    r = rate_0;
    i = ARRAY_SIZE(rate_0);
  } else {
    r = rate_1;
    i = ARRAY_SIZE(rate_1);
  }
  while(--i > 0){
    if(frame_rate >= r->fps){
            break;
    }
    r++;
  }

  sccb_reg_write(0x11, r->r11);
  sccb_reg_write(0x0d, r->r0d);
  ov534_reg_write(0xe5, r->re5);

  ltr_int_log_message("frame_rate: %d\n", r->fps);
}


static void sethue(int32_t val)
{
  int16_t huesin;
  int16_t huecos;

  // According to the datasheet the registers expect HUESIN and
  // HUECOS to be the result of the trigonometric functions,
  // scaled by 0x80.
  //
  // The 0x7fff here represents the maximum absolute value
  // returned byt fixp_sin and fixp_cos, so the scaling will
  // consider the result like in the interval [-1.0, 1.0].
  //
  huesin = sinf(val) * 0x80;
  huecos = cosf(val) * 0x80;

  if (huesin < 0) {
    sccb_reg_write(0xab, sccb_reg_read(0xab) | 0x2);
    huesin = -huesin;
  } else {
    sccb_reg_write(0xab, sccb_reg_read(0xab) & ~0x2);
  }
  sccb_reg_write(0xa9, (uint8_t)huecos);
  sccb_reg_write(0xaa, (uint8_t)huesin);
}

static void setsaturation(int32_t val)
{
  sccb_reg_write(0xa7, val); // U saturation *
  sccb_reg_write(0xa8, val); // V saturation *
}

static void setbrightness(int32_t val)
{
  sccb_reg_write(0x9b, val);
}

static void setcontrast(int32_t val)
{
  sccb_reg_write(0x9c, val);
}

static void setgain(int32_t val)
{
  switch(val & 0x30){
    case 0x00:
      val &= 0x0f;
      break;
    case 0x10:
      val &= 0x0f;
      val |= 0x30;
      break;
    case 0x20:
      val &= 0x0f;
      val |= 0x70;
      break;
    default:
      val &= 0x0f;
      val |= 0xf0;
      break;
  }
  sccb_reg_write(0x00, val);
}

/*
static int32_t getgain()
{
  return sccb_reg_read(0x00);
}
*/

static void setexposure(int32_t val)
{
  // 'val' is one byte and represents half of the exposure value
  // we are going to set into registers, a two bytes value:
  //
  //    MSB: ((u16) val << 1) >> 8   == val >> 7
  //    LSB: ((u16) val << 1) & 0xff == val << 1
  //
  sccb_reg_write(0x08, val >> 7);
  sccb_reg_write(0x10, val << 1);
}

/*
static int32_t getexposure()
{
  uint8_t hi = sccb_reg_read(0x08);
  uint8_t lo = sccb_reg_read(0x10);
  return (hi << 8 | lo) >> 1;
}
*/

static void setagc(int32_t val)
{
  if(val){
    sccb_reg_write(0x13, sccb_reg_read(0x13) | 0x04);
    sccb_reg_write(0x64, sccb_reg_read(0x64) | 0x03);
  } else {
    sccb_reg_write(0x13, sccb_reg_read(0x13) & ~0x04);
    sccb_reg_write(0x64, sccb_reg_read(0x64) & ~0x03);
  }
}

static void setawb(int32_t val)
{
  if(val){
    sccb_reg_write(0x13, sccb_reg_read(0x13) | 0x02);
    sccb_reg_write(0x63, sccb_reg_read(0x63) | 0xc0);
  }else{
    sccb_reg_write(0x13, sccb_reg_read(0x13) & ~0x02);
    sccb_reg_write(0x63, sccb_reg_read(0x63) & ~0xc0);
  }
}

static void setaec(int32_t val)
{
  uint8_t data = 0x01;
  switch (val) {
    case V4L2_EXPOSURE_AUTO:
      sccb_reg_write(0x13, sccb_reg_read(0x13) | data);
      break;
    case V4L2_EXPOSURE_MANUAL:
      sccb_reg_write(0x13, sccb_reg_read(0x13) & ~data);
      break;
  }
}

static void setsharpness(int32_t val)
{
  sccb_reg_write(0x91, val);   // Auto de-noise threshold
  sccb_reg_write(0x8e, val);   // De-noise threshold
}

static void sethvflip(int32_t hflip, int32_t vflip)
{
  uint8_t val;

  val = sccb_reg_read(0x0c);
  val &= ~0xc0;
  if(hflip == 0){
    val |= 0x40;
  }
  if(vflip == 0){
    val |= 0x80;
  }
  sccb_reg_write(0x0c, val);
}

static void setlightfreq(int32_t val)
{
  val = val ? 0x9e : 0x00;
  sccb_reg_write(0x2b, val);
}

/*


static int ov534_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
        struct sd *sd = container_of(ctrl->handler, struct sd, ctrl_handler);
        struct gspca_dev *gspca_dev = &sd->gspca_dev;

        switch (ctrl->id) {
        case V4L2_CID_AUTOGAIN:
                gspca_dev->usb_err = 0;
                if (ctrl->val && sd->gain && gspca_dev->streaming)
                        sd->gain->val = getgain(gspca_dev);
                return gspca_dev->usb_err;

        case V4L2_CID_EXPOSURE_AUTO:
                gspca_dev->usb_err = 0;
                if (ctrl->val == V4L2_EXPOSURE_AUTO && sd->exposure &&
                    gspca_dev->streaming)
                        sd->exposure->val = getexposure(gspca_dev);
                return gspca_dev->usb_err;
        }
        return -EINVAL;
}

static int ov534_s_ctrl(struct v4l2_ctrl *ctrl)
{
        struct sd *sd = container_of(ctrl->handler, struct sd, ctrl_handler);
        struct gspca_dev *gspca_dev = &sd->gspca_dev;

        gspca_dev->usb_err = 0;
        if (!gspca_dev->streaming)
                return 0;

        switch (ctrl->id) {
        case V4L2_CID_HUE:
                sethue(gspca_dev, ctrl->val);
                break;
        case V4L2_CID_SATURATION:
                setsaturation(gspca_dev, ctrl->val);
                break;
        case V4L2_CID_BRIGHTNESS:
                setbrightness(gspca_dev, ctrl->val);
                break;
        case V4L2_CID_CONTRAST:
                setcontrast(gspca_dev, ctrl->val);
                break;
        case V4L2_CID_AUTOGAIN:
        // case V4L2_CID_GAIN:
                setagc(gspca_dev, ctrl->val);
                if (!gspca_dev->usb_err && !ctrl->val && sd->gain)
                        setgain(gspca_dev, sd->gain->val);
                break;
        case V4L2_CID_AUTO_WHITE_BALANCE:
                setawb(gspca_dev, ctrl->val);
                break;
        case V4L2_CID_EXPOSURE_AUTO:
        // case V4L2_CID_EXPOSURE:
                setaec(gspca_dev, ctrl->val);
                if (!gspca_dev->usb_err && ctrl->val == V4L2_EXPOSURE_MANUAL &&
                    sd->exposure)
                        setexposure(gspca_dev, sd->exposure->val);
                break;
        case V4L2_CID_SHARPNESS:
                setsharpness(gspca_dev, ctrl->val);
                break;
        case V4L2_CID_HFLIP:
                sethvflip(gspca_dev, ctrl->val, sd->vflip->val);
                break;
        case V4L2_CID_VFLIP:
                sethvflip(gspca_dev, sd->hflip->val, ctrl->val);
                break;
        case V4L2_CID_POWER_LINE_FREQUENCY:
                setlightfreq(gspca_dev, ctrl->val);
                break;
        }
        return gspca_dev->usb_err;
}

static const struct v4l2_ctrl_ops ov534_ctrl_ops = {
        .g_volatile_ctrl = ov534_g_volatile_ctrl,
        .s_ctrl = ov534_s_ctrl,
};

static int sd_init_controls(struct gspca_dev *gspca_dev)
{
        struct sd *sd = (struct sd *) gspca_dev;
        struct v4l2_ctrl_handler *hdl = &sd->ctrl_handler;
        // parameters with different values between the supported sensors
        int saturation_min;
        int saturation_max;
        int saturation_def;
        int brightness_min;
        int brightness_max;
        int brightness_def;
        int contrast_max;
        int contrast_def;
        int exposure_min;
        int exposure_max;
        int exposure_def;
        int hflip_def;

                saturation_min = 0,
                saturation_max = 255,
                saturation_def = 64,
                brightness_min = 0;
                brightness_max = 255;
                brightness_def = 0;
                contrast_max = 255;
                contrast_def = 32;
                exposure_min = 0;
                exposure_max = 255;
                exposure_def = 120;
                hflip_def = 0;


                sd->hue = v4l2_ctrl_new_std(hdl, &ov534_ctrl_ops,
                                V4L2_CID_HUE, -90, 90, 1, 0);

        sd->saturation = v4l2_ctrl_new_std(hdl, &ov534_ctrl_ops,
                        V4L2_CID_SATURATION, saturation_min, saturation_max, 1,
                        saturation_def);
        sd->brightness = v4l2_ctrl_new_std(hdl, &ov534_ctrl_ops,
                        V4L2_CID_BRIGHTNESS, brightness_min, brightness_max, 1,
                        brightness_def);
        sd->contrast = v4l2_ctrl_new_std(hdl, &ov534_ctrl_ops,
                        V4L2_CID_CONTRAST, 0, contrast_max, 1, contrast_def);

        if (sd->sensor == SENSOR_OV772x) {
                sd->autogain = v4l2_ctrl_new_std(hdl, &ov534_ctrl_ops,
                                V4L2_CID_AUTOGAIN, 0, 1, 1, 1);
                sd->gain = v4l2_ctrl_new_std(hdl, &ov534_ctrl_ops,
                                V4L2_CID_GAIN, 0, 63, 1, 20);
        }

        sd->autoexposure = v4l2_ctrl_new_std_menu(hdl, &ov534_ctrl_ops,
                        V4L2_CID_EXPOSURE_AUTO,
                        V4L2_EXPOSURE_MANUAL, 0,
                        V4L2_EXPOSURE_AUTO);
        sd->exposure = v4l2_ctrl_new_std(hdl, &ov534_ctrl_ops,
                        V4L2_CID_EXPOSURE, exposure_min, exposure_max, 1,
                        exposure_def);

        sd->autowhitebalance = v4l2_ctrl_new_std(hdl, &ov534_ctrl_ops,
                        V4L2_CID_AUTO_WHITE_BALANCE, 0, 1, 1, 1);

        if (sd->sensor == SENSOR_OV772x)
                sd->sharpness = v4l2_ctrl_new_std(hdl, &ov534_ctrl_ops,
                                V4L2_CID_SHARPNESS, 0, 63, 1, 0);

        sd->hflip = v4l2_ctrl_new_std(hdl, &ov534_ctrl_ops,
                        V4L2_CID_HFLIP, 0, 1, 1, hflip_def);
        sd->vflip = v4l2_ctrl_new_std(hdl, &ov534_ctrl_ops,
                        V4L2_CID_VFLIP, 0, 1, 1, 0);
        sd->plfreq = v4l2_ctrl_new_std_menu(hdl, &ov534_ctrl_ops,
                        V4L2_CID_POWER_LINE_FREQUENCY,
                        V4L2_CID_POWER_LINE_FREQUENCY_50HZ, 0,
                        V4L2_CID_POWER_LINE_FREQUENCY_DISABLED);

        if (hdl->error) {
                pr_err("Could not initialize controls\n");
                return hdl->error;
        }

        if (sd->sensor == SENSOR_OV772x)
                v4l2_ctrl_auto_cluster(2, &sd->autogain, 0, true);

        v4l2_ctrl_auto_cluster(2, &sd->autoexposure, V4L2_EXPOSURE_MANUAL,
                               true);

        return 0;
}
*/

static void sd_stopN(void)
{
  ov534_reg_write(0xe0, 0x09);
  ov534_set_led(0);
}

// this function is called at probe and resume time
int sd_init(void)
{
        uint16_t sensor_id;

        // reset bridge
        ov534_reg_write(0xe7, 0x3a);
        ov534_reg_write(0xe0, 0x08);
        usleep(100000);

        // initialize the sensor address
        ov534_reg_write(OV534_REG_ADDRESS, 0x42);

        // reset sensor
        sccb_reg_write(0x12, 0x80);
        usleep(10000);

        // probe the sensor
        sccb_reg_read(0x0a);
        sensor_id = sccb_reg_read(0x0a) << 8;
        sccb_reg_read(0x0b);
        sensor_id |= sccb_reg_read(0x0b);
        ltr_int_log_message("Sensor ID: %04x\n", sensor_id);

        // initialize
        reg_w_array(bridge_init_772x, ARRAY_SIZE(bridge_init_772x));
        ov534_set_led(1);
        sccb_w_array(sensor_init_772x,
                        ARRAY_SIZE(sensor_init_772x));

        sd_stopN();
//      set_frame_rate(v);

        return usb_err;
}

struct reg_array {
        const uint8_t (*val)[2];
        size_t len;
};


static int width = 320;
static int height = 240;

static int sd_start(int mode)
{
        int frame_rate = ltr_int_ps3_get_ctrl_val(e_FPS);
        if(mode > 1){
          ltr_int_log_message("Wrong mode %d selected. Defaulting to 320x240.\n", mode);
          mode = 0;
        }
        static const struct reg_array bridge_start[2] = {
          {bridge_start_qvga_772x, ARRAY_SIZE(bridge_start_qvga_772x)},
          {bridge_start_vga_772x, ARRAY_SIZE(bridge_start_vga_772x)}
        };
        static const struct reg_array sensor_start[2] = {
          {sensor_start_qvga_772x, ARRAY_SIZE(sensor_start_qvga_772x)},
          {sensor_start_vga_772x, ARRAY_SIZE(sensor_start_vga_772x)}
        };

        curr_mode = mode;    // 0: 320x240, 1: 640x480
        if(mode == 0){
          width  = 320;
          height = 240;
        }else{
          width  = 640;
          height = 480;
        }
        reg_w_array(bridge_start[mode].val,
                    bridge_start[mode].len);
        sccb_w_array(sensor_start[mode].val,
                     sensor_start[mode].len);

        set_frame_rate(frame_rate);

        sethue(ltr_int_ps3_get_ctrl_val(e_HUE));
        setsaturation(ltr_int_ps3_get_ctrl_val(e_SATURATION));
        setagc(ltr_int_ps3_get_ctrl_val(e_AUTOGAIN));
        setawb(ltr_int_ps3_get_ctrl_val(e_AUTOWHITEBALANCE));
        setaec(ltr_int_ps3_get_ctrl_val(e_AUTOEXPOSURE));
        setgain(ltr_int_ps3_get_ctrl_val(e_GAIN));
        setexposure(ltr_int_ps3_get_ctrl_val(e_EXPOSURE));
        setbrightness(ltr_int_ps3_get_ctrl_val(e_BRIGHTNESS));
        setcontrast(ltr_int_ps3_get_ctrl_val(e_CONTRAST));
        setsharpness(ltr_int_ps3_get_ctrl_val(e_SHARPNESS));
        sethvflip(ltr_int_ps3_get_ctrl_val(e_HFLIP),
                  ltr_int_ps3_get_ctrl_val(e_VFLIP));
        setlightfreq(ltr_int_ps3_get_ctrl_val(e_PLFREQ));

        ov534_set_led(1);
        ov534_reg_write(0xe0, 0x00);
        return usb_err;
}

static bool ltr_int_refresh_ctrls(void)
{
  if(!ltr_int_ps3_controls_changed()){
    return true;
  }
  int i;
  for(i = 0; i < e_NUMCTRLS; ++i){
    if(ltr_int_ps3_ctrl_changed(i)){
      int val = ltr_int_ps3_get_ctrl_val(i);
      switch(i){
        case e_HUE:
          sethue(val);
          break;
        case e_SATURATION:
          setsaturation(val);
          break;
        case e_AUTOGAIN:
          setagc(val);
          break;
        case e_AUTOWHITEBALANCE:
          setawb(val);
          break;
        case e_AUTOEXPOSURE:
          setaec(val);
          break;
        case e_GAIN:
          setgain(val);
          break;
        case e_EXPOSURE:
          setexposure(val);
          break;
        case e_BRIGHTNESS:
          setbrightness(val);
          break;
        case e_CONTRAST:
          setcontrast(val);
          break;
        case e_SHARPNESS:
          setsharpness(val);
          break;
        case e_HFLIP:
          sethvflip(ltr_int_ps3_get_ctrl_val(e_HFLIP), ltr_int_ps3_get_ctrl_val(e_VFLIP));
          break;
        case e_VFLIP:
          sethvflip(ltr_int_ps3_get_ctrl_val(e_HFLIP), ltr_int_ps3_get_ctrl_val(e_VFLIP));
          break;
        case e_PLFREQ:
          setlightfreq(val);
          break;
        case e_FPS:
          //intentionaly empty - fps is set when starting
          break;
        default:
          ltr_int_log_message("PS3 eye: Attempt to set nonexistent control %d.\n");
          break;
      }
    }
  }
  return 0;
}


// Values for bmHeaderInfo (Video and Still Image Payload Headers, 2.4.3.3)
#define UVC_STREAM_EOH  (1 << 7)
#define UVC_STREAM_ERR  (1 << 6)
#define UVC_STREAM_STI  (1 << 5)
#define UVC_STREAM_RES  (1 << 4)
#define UVC_STREAM_SCR  (1 << 3)
#define UVC_STREAM_PTS  (1 << 2)
#define UVC_STREAM_EOF  (1 << 1)
#define UVC_STREAM_FID  (1 << 0)

typedef enum {
  DISCARD_PACKET,
  FIRST_PACKET,
  INTER_PACKET,
  LAST_PACKET
} packet_type_t;


static uint32_t last_pts;
static uint16_t last_fid;
static packet_type_t last_packet_type;

int min(int a, int b)
{
  return (a < b) ? a : b;
}

static int frame_max = -1;
static uint8_t *frame1 = NULL;
static int pos = 0;
static int frame_counter = 0;

static void frame_add(int packet_type, uint8_t *data, int len)
{
  if((frame1 == NULL) || (frame_max != width * height * 2)){
    if(frame1 != NULL){
      free(frame1);
    }
    frame_max = width * height * 2;
    frame1 = (uint8_t*)malloc(frame_max);
  }
  if((packet_type == DISCARD_PACKET) || (packet_type == FIRST_PACKET)){
    pos = 0;
  }
  if(packet_type == DISCARD_PACKET){
    return;
  }
  if((data != NULL) && (len > 0)){
    if(pos + len > frame_max){
      ltr_int_log_message("Frame way too long (%d is max, requested %d)!\n",
                            frame_max, pos + len);
      pos = 0;
      return;
    }
    memcpy(frame1 + pos, data, len);
    pos += len;
  }
  if(packet_type == LAST_PACKET){
    //process_packet
    ++frame_counter;
  }
  return;
}


static void sd_pkt_scan(uint8_t *data, int len)
{
  uint32_t this_pts;
  uint16_t this_fid;
  int remaining_len = len;
  int payload_len;

  payload_len = 2048;
  do {
    len = min(remaining_len, payload_len);

    // Payloads are prefixed with a UVC-style header.  We
    // consider a frame to start when the FID toggles, or the PTS
    // changes.  A frame ends when EOF is set, and we've received
    // the correct number of bytes.

    // Verify UVC header.  Header length is always 12
    if (data[0] != 12 || len < 12) {
      ltr_int_log_message("bad header\n");
      printf("bad header\n");
      goto discard;
    }

    // Check errors
    if (data[1] & UVC_STREAM_ERR) {
      ltr_int_log_message("payload error\n");
      goto discard;
    }

    // Extract PTS and FID
    if (!(data[1] & UVC_STREAM_PTS)) {
      ltr_int_log_message("PTS not present\n");
      goto discard;
    }
    this_pts = (data[5] << 24) | (data[4] << 16) | (data[3] << 8) | data[2];
    this_fid = (data[1] & UVC_STREAM_FID) ? 1 : 0;

    // If PTS or FID has changed, start a new frame.
    if (this_pts != last_pts || this_fid != last_fid) {
      if(last_packet_type == INTER_PACKET){
        frame_add(LAST_PACKET, NULL, 0);
      }
      last_pts = this_pts;
      last_fid = this_fid;
      frame_add(FIRST_PACKET, data + 12, len - 12);
    // If this packet is marked as EOF, end the frame
    }else if(data[1] & UVC_STREAM_EOF){
      last_pts = 0;
      if(pos + len - 12 != width * height * 2) {
        ltr_int_log_message("wrong sized frame (%d vs %d)\n", pos + len - 12, width * height * 2);
        goto discard;
      }
      frame_add(LAST_PACKET, data + 12, len - 12);
    }else{
      // Add the data from this payload
      frame_add(INTER_PACKET, data + 12, len - 12);
    }

    // Done this payload
    goto scan_next;

discard:
    // Discard data until a new frame starts.
    last_packet_type = DISCARD_PACKET;

scan_next:
    remaining_len -= len;
    data += len;
  } while (remaining_len > 0);
}

static unsigned int threshold = 128;

static void get_bw_image(const unsigned char *source_buf, unsigned char *dest_buf, unsigned int bytes_used)
{
  unsigned int cntr, cntr1;
  #ifdef OPENCV
    threshold = 0;
  #else
    threshold = ltr_int_ps3_get_threshold();
  #endif
  for(cntr = cntr1 = 0; cntr < bytes_used; cntr += 2, ++cntr1){
    if(source_buf[cntr] > threshold){
      dest_buf[cntr1] = source_buf[cntr];
    }else{
      dest_buf[cntr1] = 0;
    }
  }
}

static int w, h;

int ltr_int_tracker_resume(void)
{
  int mode = 0; //default 320x240
  if((w == 640) && (h == 480)){
    mode = 1;
  }
  sd_start(mode);
  return 0;
}

int ltr_int_tracker_init(struct camera_control_block *ccb)
{
  (void) ccb;
  usb_err = 0;
  if(!ltr_int_init_usb()){
    ltr_int_log_message("Failed to initialize usb!\n");
    return -1;
  }

  if(!ltr_int_find_p3e()){
    ltr_int_log_message("Can't find the Ps3Eye!\n");
    goto failed;
  }

  if(!ltr_int_prepare_device(1, 0)){
    ltr_int_log_message("Couldn't prepare!\n");
    goto failed;
  }

  if(sd_init()){
    goto failed;
  }
  if(!ltr_int_ps3_init_prefs()){
    goto failed;
  }
  if(!ltr_int_ps3_get_resolution(&w, &h)){
    goto failed;
  }
  ltr_int_prepare_for_processing(w, h);
#ifdef OPENCV
  if(!ltr_int_init_face_detect()){
    ltr_int_log_message("Couldn't initialize facetracking!\n");
  }
#endif
  return ltr_int_tracker_resume();

 failed:
  ltr_int_finish_usb(-1);
  return -1;
}

int ltr_int_tracker_close(void)
{
  sd_stopN();
#ifdef OPENCV
  ltr_int_stop_face_detect();
#endif
  ltr_int_cleanup_after_processing();
  ltr_int_finish_usb(-1);
  return 0;
}

int ltr_int_tracker_pause(void)
{
  sd_stopN();
  return 0;
}


static uint8_t buffer[16384];
static int current_frame = 0;
static unsigned char *frame = NULL;


int ltr_int_tracker_get_frame(struct camera_control_block *ccb, struct frame_type *f,
                              bool *frame_acquired)
{
  (void) ccb;
  *frame_acquired = false;
  f->width = w;
  f->height = h;
  ltr_int_refresh_ctrls();

  //bulk transfer, ep 81
  size_t got = 0;
  if(!ltr_int_receive_data(0x81, buffer, sizeof(buffer), &got, 500)){
    return -1;
  }
  sd_pkt_scan(buffer, got);
  if(frame_counter != current_frame){
    //printf("Have new frame!\n");
    current_frame = frame_counter;

    if(frame == NULL){
      frame = (unsigned char *)malloc(w * h);
    }
    const unsigned char *source_buf = frame1;

    FILE *fr = fopen("/tmp/frame.bin", "w");
    if(fr){
      fwrite(source_buf, 2 * w * h, 1, fr);
      fclose(fr);
    }

    unsigned char *dest_buf = (f->bitmap != NULL) ? f->bitmap : frame;
    get_bw_image(source_buf, dest_buf, 2 * w * h);

    image_t img;
    img.bitmap = dest_buf;
    img.w = f->width;
    img.h = f->height;
    img.ratio = 1.0f;

#ifndef OPENCV
    ltr_int_to_stripes(&img);
    ltr_int_stripes_to_blobs(MAX_BLOBS, &(f->bloblist), ltr_int_ps3_get_min_blob(),
                    ltr_int_ps3_get_max_blob(), &img);
#else
    ltr_int_face_detect(&img, &(f->bloblist));
#endif
     *frame_acquired = true;
  }
  return 0;
}

int ltr_int_ps3eye_found(void)
{
  if(!ltr_int_init_usb()){
    ltr_int_log_message("Failed to initialize usb!\n");
    return 0;
  }
  bool res = ltr_int_find_p3e();
  ltr_int_finish_usb(-1);
  return res;
}

