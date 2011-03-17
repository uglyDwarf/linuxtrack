#! /usr/bin/python
# -*- coding: utf-8 -*-
##bc##################################################################
## (C) Copyright 2009, All Rights Reserved.
##
## Name         : tir4.py
## Author       : DT Austin
## Created      : 07/02/2009
## SVN date     : $Date$
##
######################################################################
## Description: python device driver for the TIR4 device
##ec##################################################################

import sys
import time
try:
  import usb
except:
  print("ERROR: python lib USB missing!")
  sys.exit(1)
try:
    import bulk_config_data
except:
    print("ERROR: bulk_config_data missing!")
    sys.exit(1)
# public Constants
TIR_VENDOR_ID = 0x131d
TIR_PRODUCT_ID = 0x0156
CROPPED_NUM_VLINES = 288
CROPPED_NUM_HPIX = 710
RAW_NUM_VLINES = 512
RAW_NUM_HPIX = 1024
FRAME_QUEUE_MAX_DEPTH = 2
# #define v4l2_fourcc(a,b,c,d) (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))
# #define V4L2_PIX_FMT_TIR4   v4l2_fourcc('T','I','R','4') /* TIR4 compress */
# #define V4L2_PIX_FMT_RGB332  v4l2_fourcc('R','G','B','1') /*  8  RGB-3-3-2   */
V4L2_PIX_FMT_TIR4 = "TIR4"
V4L2_PIX_FMT_GREY = "RGB1"
# private Constants
NOP_MSGLEN = 0XEF
TBD0_MSGLEN = 0X07
TBD0_MSGID = 0X20
VALID_MIN_MSGLEN = 0x02
VALID_MAX_MSGLEN = 0x3E
VALID_MSGID = 0x1c
DEVICE_STRIPE_LEN = 4
VSYNC_DEVICE_STRIPE = (0x00, 0x00, 0x00, 0x00)
STRIPE_LEN = 3
TIR_INTERFACE_ID = 0
TIR_BULK_IN_EP = 0x82
TIR_BULK_OUT_EP = 0x01
TIR_CONFIGURATION = 0x01
TIR_ALTINTERFACE = 0x00
LINE_NUM_0X100_BIT_MASK = 0X20
START_PIX_0X100_BIT_MASK = 0X80
START_PIX_0X200_BIT_MASK = 0X10
STOP_PIX_0X100_BIT_MASK = 0X40
STOP_PIX_0X200_BIT_MASK = 0X08
BULK_READ_SIZE = 0X4000
BULK_READ_TIMEOUT = 20  # milliseconds
BULK_WRITE_TIMEOUT = 1000 # milliseconds
TIR_LED_MSGID = 0x10
TIR_IR_LED_BIT_MASK = 0x80
TIR_GREEN_LED_BIT_MASK = 0x20
TIR_RED_LED_BIT_MASK = 0x10
TIR_BLUE_LED_BIT_MASK = 0x40
TIR_ALL_LED_BIT_MASK = TIR_IR_LED_BIT_MASK | TIR_GREEN_LED_BIT_MASK | TIR_RED_LED_BIT_MASK | TIR_BLUE_LED_BIT_MASK
READ_DISCONNECT_TIMEOUT = 2 # seconds

# private Static members
vline_offset = 12
hpix_offset = 80
crop_frames = True

# public data structures
class Enumeration(object):
  def __init__(self, names):
    for number, name in enumerate(names):
      setattr(self, name, number)

TIR4EXCEPTION_ENUM = Enumeration(("USB_LIST_FAILED",
                                  "FIND_DEVICE_FAILED",
                                  "CREATE_HANDLE_FAILED",
                                  "CLAIM_FAILED",
                                  "DISCONNECT",
                                  "UNKNOWN_READ_ERROR",
                                  "UNKNOWN_PACKET"))
class TIR4Exception(Exception):
  def __init__(self, ID):
    self.args = (ID,)

class TIR4Control(object):
  def __init__(self):
    self.do_reset()
  def do_reset(self):
    self.readbyteq = ByteQueue()
    self.writebyteq = ByteQueue()
    self.mp = MessageProcessor()
    self.init_step_5percent_callback = None
    self.device_notpresent_callback = None
    self.on_read_disconnect_watch = False
    self.first_read_missing_timestamp = 0
  def is_device_present(self):
    return (self.find_device() != None)
  def find_device(self):
    buses = usb.busses()
    if not(buses):
        raise TIR4Exception(TIR4EXCEPTION_ENUM.USB_LIST_FAILED)
    for bus in buses:
        for device in bus.devices:
            if device.idVendor == TIR_VENDOR_ID:
                if device.idProduct == TIR_PRODUCT_ID:
                    return device
    return None
  def do_full_init(self):
    self.do_init_step_start()
    while not self.is_init_step_done():
      self.do_init_step()
  def do_init_step_start(self):
    self.device = self.find_device()
    if not(self.device):
      raise TIR4Exception(TIR4EXCEPTION_ENUM.FIND_DEVICE_FAILED)
    self.device_handle = self.device.open()
    if not(self.device_handle):
      raise TIR4Exception(TIR4EXCEPTION_ENUM.CREATE_HANDLE_FAILED)
    try:
      self.device_handle.claimInterface(TIR_INTERFACE_ID)
    except usb.USBError:
      raise TIR4Exception(TIR4EXCEPTION_ENUM.CLAIM_FAILED)
    self.device_handle.setAltInterface(TIR_ALTINTERFACE)
    self.desc = self.device_handle.getDescriptor(0x0000002, # type
                                                 0x0000000, # index
                                                 0x0000009) # length
    self.device_handle.releaseInterface()
    self.device_handle.setConfiguration(TIR_CONFIGURATION)
    self.device_handle.claimInterface(TIR_INTERFACE_ID)
    self.device_handle.setAltInterface(TIR_ALTINTERFACE)
    self.bulk_config_len = len(bulk_config_data.bulk_config)
    self.bulk_config_index = 0
    self.five_percent_current_thresh = 5
  def do_init_step(self):
    if not(self.is_init_step_done()):
      packet = bulk_config_data.bulk_config[self.bulk_config_index]
      self.nq_write_usb(packet)
      self.bulk_config_index += 1
      if self.init_step_5percent_callback != None:
        if self.get_init_step_percent_complete() > self.five_percent_current_thresh:
          self.five_percent_current_thresh += 5
          self.init_step_5percent_callback()
  def is_init_step_done(self):
    return (self.bulk_config_len == self.bulk_config_index)
  def get_init_step_percent_complete(self):
    return 100.0*self.bulk_config_index/self.bulk_config_len
  def set_init_step_5percent_callback(self,func):
    self.init_step_5percent_callback = func
  def do_read_usb(self):
    try:
      readbytes = self.device_handle.bulkRead(TIR_BULK_IN_EP,
                                              BULK_READ_SIZE,
                                              BULK_READ_TIMEOUT)
      self.readbyteq.append_bytes(readbytes)
      self.on_read_disconnect_watch = False
    except usb.USBError, errorcode:
      if errorcode.args == ('No error',):
        if self.on_read_disconnect_watch:
          if (time.clock()-self.first_read_missing_timestamp) > READ_DISCONNECT_TIMEOUT:
            raise TIR4Exception(TIR4EXCEPTION_ENUM.DISCONNECT)
          else:
            pass # continue on
        else:
          self.first_read_missing_timestamp = time.clock()
          self.on_read_disconnect_watch = True
      elif errorcode.args == ('error reaping URB: No such device',):
        raise TIR4Exception(TIR4EXCEPTION_ENUM.DISCONNECT)
      else:
        print errorcode.args
        raise TIR4Exception(TIR4EXCEPTION_ENUM.UNKNOWN_READ_ERROR)
  def do_write_usb_queued(self):
    bytes_written = self.device_handle.bulkWrite(TIR_BULK_OUT_EP,
                                                 self.writebyteq.peek_bytes(),
                                                 BULK_WRITE_TIMEOUT)
    self.writebyteq.drop_bytes(bytes_written)
  def nq_write_usb(self, buf):
    self.writebyteq.append_bytes(buf)
    self.do_write_usb_queued()
  def process_readbyteq(self):
    self.mp.add_bytes(self.readbyteq.pop_bytes())
  def peek_frames(self):
    return self.mp.get_frameq().peek_frames()
  def is_frame_available(self):
    return not(self.mp.get_frameq().is_empty())
  def pop_frame(self):
    return self.mp.get_frameq().pop()
  def set_vline_offset(self, offset):
    global vline_offset
    vline_offset = offset
  def set_hpix_offset(self, offset):
    global hpix_offset
    hpix_offset = offset
  def get_vline_offset(self):
    global vline_offset
    return vline_offset
  def get_hpix_offset(self):
    global hpix_offset
    return hpix_offset
  def init_leds(self):
    self.set_all_led_off()
  def set_all_led_off(self):
    self.set_led_worker(0,
                        TIR_ALL_LED_BIT_MASK)
    self.ir_led_on = False
    self.green_led_on = False
    self.red_led_on = False
    self.blue_led_on = False
  def set_ir_led_on(self, arg):
    if arg:
      cmd = TIR_IR_LED_BIT_MASK
    else:
      cmd = 0
    self.set_led_worker(cmd,
                        TIR_IR_LED_BIT_MASK)
    self.ir_led_on = arg
  def set_green_led_on(self, arg):
    if arg:
      cmd = TIR_GREEN_LED_BIT_MASK
    else:
      cmd = 0
    self.set_led_worker(cmd,
                        TIR_GREEN_LED_BIT_MASK)
    self.green_led_on = arg
  def set_red_led_on(self, arg):
    if arg:
      cmd = TIR_RED_LED_BIT_MASK
    else:
      cmd = 0
    self.set_led_worker(cmd,
                        TIR_RED_LED_BIT_MASK)
    self.red_led_on = arg
  def set_blue_led_on(self, arg):
    if arg:
      cmd = TIR_BLUE_LED_BIT_MASK
    else:
      cmd = 0
    self.set_led_worker(cmd,
                        TIR_BLUE_LED_BIT_MASK)
    self.blue_led_on = arg
  def is_ir_led_on(self, arg):
    return self.ir_led_on
  def is_green_led_on(self, arg):
    return self.green_led_on
  def is_red_led_on(self, arg):
    return self.red_led_on
  def is_blue_led_on(self, arg):
    return self.blue_led_on
  def set_led_worker(self, cmd, mask):
    self.nq_write_usb((TIR_LED_MSGID,
                       cmd,
                       mask))
  def set_device_notpresent_callback(self, func):
    self.device_notpresent_callback = func
  def set_crop_frames(self, arg):
    global crop_frames
    crop_frames = arg
  def is_crop_frames(self):
    global crop_frames
    return crop_frames
  def trim(self):
    self.mp.trim()
  def set_frame_format(self):
    #TBD
    pass
  def get_frame_format(self):
    #TBD
    pass

# stripes must be added in vline sorted order!
class Blob(list):
  def __init__(self,stripe=None):
    list.__init__(self)
    self.area = 0
    if stripe != None:
      self.append(stripe)
  def __cmp__(self,other):
    return cmp(other.get_area(), self.get_area())
  def append(self, stripe):
    list.append(self,stripe)
    self.area += (stripe.hstop-stripe.hstart)
  def extend(self, blob):
    list.extend(self,blob)
    self.area += blob.get_area()
  def head(self):
    return self[0]
  def tail(self):
    return self[len(self)-1]
  def is_contact(self,arg_stripe):
    for self_stripe in reversed(self):
      if self_stripe.vline < arg_stripe.vline - 2:
        return False
      elif self_stripe.is_h_contact(arg_stripe):
        return True
    return False
  def get_center_coords(self):
    self.cum_line_area_product = 0
    self.cum_2x_hcenter_area_product = 0
    for stripe in self:
      area = (stripe.hstop-stripe.hstart)
      self.cum_line_area_product += stripe.vline*area
      self.cum_2x_hcenter_area_product += (stripe.hstop+stripe.hstart)*area
    if self.area > 0:
      self.vcenter = 1.0*self.cum_line_area_product / self.area
      self.hcenter = 1.0*self.cum_2x_hcenter_area_product / self.area / 2
    else:
      self.vcenter = 1.0*self.cum_line_area_product / (self.area+0.001)
      self.hcenter = 1.0*self.cum_2x_hcenter_area_product / (self.area+0.001) / 2
    return (self.vcenter, self.hcenter)
  def get_area(self):
    return self.area
  
  def __str__(self):
    returnstr = ""
    returnstr += "------ Blob Start ------\n"
    for stripe in self:
      returnstr += " " + str(stripe) + "\n"
    returnstr += "------  Blob End  ------\n"
    return returnstr

class Frame(list):
  def __init__(self,device_stripes=None):
    list.__init__(self)
    if device_stripes:
      for device_stripe in device_stripes:
        stripe = device_stripe.do_xlate_device_stripe()
        self.append(stripe)
  def __str__(self):
    returnstr = ""
    returnstr += "------ Frame Start ------\n"
    for stripe in self:
      returnstr += " " + str(stripe) + "\n"
    returnstr += "------  Frame End  ------\n"
    return returnstr
  def find_blobs(self):
    # fix the ^ case
    open_blobs = []
    closed_blobs = []
    for stripe in self:
      # find open blobs that match this stripe's vline-1, 
      #   if they don't close them
      for blob in open_blobs:
        if blob.tail().vline < stripe.vline - 2:
          closed_blobs.append(blob)
          open_blobs.remove(blob)
      blob_contact_indices = []
      for i, blob in zip(range(len(open_blobs)),open_blobs):
        if blob.is_contact(stripe):
          blob_contact_indices.append(i)
      if len(blob_contact_indices) == 0:
        newblob = Blob(stripe)
        open_blobs.append(newblob)
      else:
        condensed_blob = Blob()
        newblobs = []
        for i, blob in zip(range(len(open_blobs)),open_blobs):
          if i in blob_contact_indices:
            condensed_blob.extend(blob)
          else:
            newblobs.append(blob)
        condensed_blob.append(stripe)
        newblobs.append(condensed_blob)
        open_blobs = newblobs
    result = closed_blobs + open_blobs
    result.sort(cmp=lambda x,y: x.__cmp__(y))
    return result
    
class FrameQueue(object):
  def __init__(self,frames = None):
    if frames:
      self.frames = frames
    else:
      self.frames = []
  def pop(self):
    return self.frames.pop(0)
  def append_frame(self,frame):
    self.frames.append(frame)
  def append_frames(self,frames):
    self.frames += frames
  def is_empty(self):
    return (len(self.frames) == 0)
  def peek_frames(self):
    return self.frames
  def trim(self):
    if len(self.frames) > FRAME_QUEUE_MAX_DEPTH:
      self.frames = self.frames[0:FRAME_QUEUE_MAX_DEPTH]

# private data structures
class ByteQueue(object):
  def __init__(self, bytes = None):
    if bytes:
      self.bytes = list(bytes)
    else:
      self.bytes = []
  def pop(self):
    return self.bytes.pop(0)
  def drop_bytes(self,num):
    self.bytes=self.bytes[num:len(self.bytes)]
  def pop_bytes(self):
    returnval = self.bytes
    self.bytes = []
    return returnval
  def append(self,byte):
    self.bytes.append(byte)
  def append_bytes(self,bytes):
    self.bytes += bytes
  def peek_bytes(self):
    return self.bytes
  def peek_2bytes(self):
    return (self.bytes[0], self.bytes[1])
  def __len__(self):
    return len(self.bytes)

class DeviceStripe(object):
  def __init__(self,bytes):
    self.bytes = bytes
    if len(self.bytes) != DEVICE_STRIPE_LEN:
      raise TIR4Exception("ERROR: Attempt to create a device stripe of an invalid length")
  def is_vsync(self):
    for selfbyte,vsync_byte in zip(self.bytes, VSYNC_DEVICE_STRIPE):
      if selfbyte != vsync_byte:
        return False
    return True
  def do_xlate_device_stripe(self):
    X=self.bytes[0]
    Y=self.bytes[1]
    Z=self.bytes[2]
    W=self.bytes[3]
    line_num = X
    line_num_0x100_bit = W & 0x20
    if line_num_0x100_bit != 0:
        line_num += 0x100
    start_pix = Y
    start_pix_0x100_bit = W & 0x80
    start_pix_0x200_bit = W & 0x10
    if start_pix_0x200_bit:
        start_pix += 0x200
    if start_pix_0x100_bit:
        start_pix += 0x100
    stop_pix = Z
    stop_pix_0x100_bit = W & 0x40
    stop_pix_0x200_bit = W & 0x08
    if stop_pix_0x200_bit:
        stop_pix += 0x200
    if stop_pix_0x100_bit:
        stop_pix += 0x100
    return Stripe((line_num, start_pix, stop_pix))

class Stripe(object):
  def __init__(self,init):
    if len(init) != STRIPE_LEN:
      raise TIR4Exception("ERROR: Attempt to create a stripe of an invalid length")
    self.vline = init[0]
    self.hstart = init[1]
    self.hstop = init[2]
    global crop_frames
    if crop_frames:
      self.do_crop()
  def do_crop(self):
    global vline_offset
    global hpix_offset
    self.vline -= vline_offset
    if self.vline < 0:
        self.v = 0
    if self.vline >= CROPPED_NUM_VLINES:
        self.vline = CROPPED_NUM_VLINES-1
    self.hstart -= hpix_offset
    if self.hstart < 0:
        self.hstart = 0
    if self.hstart >= CROPPED_NUM_HPIX:
        self.hstart = CROPPED_NUM_HPIX-1
    self.hstop -= hpix_offset
    if self.hstop < 0:
        self.hstop = 0
    if self.hstop >= CROPPED_NUM_HPIX:
        self.hstop = CROPPED_NUM_HPIX-1
  def is_h_contact(self,stripe):
    # tests if this stripe overlaps the argument 
    #   in the h-axis.  Vertical overlap not tested!
    # note: overlap is true if they share a single common 
    # pixel
    if self.hstop < stripe.hstart:
      return False
    elif self.hstart > stripe.hstop:
      return False
    else:
      return True
  def __str__(self):
    returnstr = ""
#     returnstr += "(0x%03x, 0x%03x, 0x%03x)" % (self.vline,
#                                                self.hstart,
#                                                self.hstop)
#     returnstr += " aka "
    returnstr += "(%03d, %04d, %04d)" % (self.vline,
                                         self.hstart,
                                         self.hstop)
    return returnstr

# turns raw usb reads into TIR4 native format frames
class MessageProcessor(object):
  def __init__(self):
    self.inbyteq = ByteQueue()
    self.outframeq = FrameQueue()
    self.pending_frame = Frame()
    self.msglen = 0
    self.msgid = -1
    self.msgcnt = 0
    self.updating = False
    self.state_enum = Enumeration(("AWAITING_HEADER",
                                   "PROCESSING_MSG",
                                   "CHOMPING_MSG"))
    self.state = self.state_enum.AWAITING_HEADER
  def add_bytes(self, bytes):
    self.inbyteq.append_bytes(bytes)
    self.updating = True
    while self.updating:
      self.process_pending_bytes()
  def process_pending_bytes(self):
    if self.state == self.state_enum.AWAITING_HEADER:
      if len(self.inbyteq) > VALID_MIN_MSGLEN:
        (self.msglen,self.msgid) = self.inbyteq.peek_2bytes()
        if self.msglen == NOP_MSGLEN:
          # chomp and continue AWAITING_HEADER
          self.inbyteq.drop_bytes(VALID_MIN_MSGLEN)
        elif (self.msglen <= VALID_MAX_MSGLEN and
              self.msglen >= VALID_MIN_MSGLEN and
              self.msgid == VALID_MSGID):
          self.msgcnt = VALID_MIN_MSGLEN
          self.inbyteq.drop_bytes(VALID_MIN_MSGLEN)
          self.state = self.state_enum.PROCESSING_MSG
        elif self.msglen == TBD0_MSGLEN and self.msgid == TBD0_MSGID:
          self.msgcnt = VALID_MIN_MSGLEN
          self.inbyteq.drop_bytes(VALID_MIN_MSGLEN)
          self.state = self.state_enum.CHOMPING_MSG
        else:
          # maybe we're off by one?
          # drop one and try again
          print "Warning READERR: 0x%02x" % self.msglen
          self.inbyteq.pop()
      else:
        self.updating = False
    elif self.state == self.state_enum.PROCESSING_MSG:
      if len(self.inbyteq) < DEVICE_STRIPE_LEN:
        self.updating = False
      elif self.msgcnt >= self.msglen:
        self.state = self.state_enum.AWAITING_HEADER
      else:
        ds = DeviceStripe((self.inbyteq.pop(),
                           self.inbyteq.pop(),
                           self.inbyteq.pop(),
                           self.inbyteq.pop()))
        self.add_device_stripe(ds)
        self.msgcnt += 4
    elif self.state == self.state_enum.CHOMPING_MSG:
      if len(self.inbyteq) == 0:
        self.updating = False
      elif self.msgcnt >= self.msglen:
        self.state = self.state_enum.AWAITING_HEADER
      else:
        byte = self.inbyteq.pop()
        self.msgcnt += 1
    else:
      self.updating = False
  def add_device_stripe(self,device_stripe):
    if device_stripe.is_vsync():
      self.outframeq.append_frame(self.pending_frame)
      self.pending_frame = Frame()
    else:
      txs = device_stripe.do_xlate_device_stripe()
      self.pending_frame.append(txs)
  def get_frameq(self):
    return self.outframeq
  def trim(self):
    self.outframeq.trim()

def runtests(argv=None):
  t4 = TIR4Control()
  print "t4.is_device_present(): ", t4.is_device_present()
  if not(t4.is_device_present()):
    sys.exit()
  t4.do_full_init()
  t4.set_all_led_off()
  t4.set_green_led_on(True)
  t4.set_ir_led_on(True)
  for i in range(0,10):
    t4.do_read_usb()
    t4.process_readbyteq()
    if t4.is_frame_available():
      frame = t4.pop_frame()
      print frame

if __name__ == "__main__":
    sys.exit(runtests(sys.argv))
