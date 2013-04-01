#import "wiimote.h"
#import <unistd.h>
#include <IOBluetooth/IOBluetoothUserLib.h>
#include <IOBluetooth/objc/IOBluetoothHostController.h>

#define LogIOReturn(result) if (result != kIOReturnSuccess) { fprintf (stderr, "IOReturn error (%s [%d]): system 0x%x, sub 0x%x, error 0x%x\n", __FILE__, __LINE__, err_get_system (result), err_get_sub (result), err_get_code (result)); }
typedef unsigned char darr[];

@implementation WiiDiscover

-(id) init
{
  self = [super init];
  if(self != nil){
    delegate = nil;
    state = IDLE;
    //Inquiry doesn't start until start method is called anyway...
    //  I just wander if it works when BT is disabled...
    inquiry = [IOBluetoothDeviceInquiry inquiryWithDelegate:self];
    if(inquiry != nil){
      [inquiry setInquiryLength:15];
      [inquiry setSearchCriteria:kBluetoothServiceClassMajorAny majorDeviceClass:0x05 minorDeviceClass:0x01];
      [inquiry setUpdateNewDeviceNames:NO];
      [inquiry retain];
    }else{
      [self release];
      self = nil;
    }
  }
  return self;
}

-(void) dealloc
{
  [inquiry stop];
  [inquiry setDelegate:nil];
  [inquiry release];
  inquiry = nil;
  [super dealloc];
}

-(id) delegate
{
  return delegate;
}

-(void) setDelegate:(id) _delegate
{
  delegate = _delegate;
}

-(IOReturn) start
{
//  if(!IOBluetoothLocalDeviceAvailable()){  //deprecated in 10.6
  if([IOBluetoothHostController defaultController] == nil){
    NSLog(@"Bluetooth capability unavailable!");
    return kIOReturnNoDevice;
  }
  if(state != IDLE){
    return kIOReturnSuccess;
  }
  if(delegate == nil){
    NSLog(@"Please select delegate first!");
    return kIOReturnNotReady;
  }
  state = REQUEST_DISCOVERY;
  [inquiry clearFoundDevices];
  IOReturn res = [inquiry start];
  if(res != kIOReturnSuccess){
    NSLog(@"Couldn't start discovery!");
    state = IDLE;
  }
  return res;
}

//IOBluetoothDeviceInquiry delegates

-(void) deviceInquiryStarted:(IOBluetoothDeviceInquiry*) sender
{
  (void) sender;
  NSLog(@"Discovery started!");
  state = DISCOVERING;
}

-(void) deviceInquiryDeviceFound:(IOBluetoothDeviceInquiry*) sender
          device:(IOBluetoothDevice*) device
{
  (void) sender;
  //We take the first wiimote found...
  if(device != nil){
    //Hopefully not needed...
    IOBluetoothIgnoreHIDDevice([device getDeviceRef]);
    [inquiry stop];
  }
}

-(void) deviceInquiryComplete:(IOBluetoothDeviceInquiry*) sender
          error:(IOReturn) error  aborted:(BOOL) aborted
{
  (void) sender;
  (void) aborted;
  id wiimote = nil;
  if(error == kIOReturnSuccess){
    NSEnumerator *i = [[inquiry foundDevices] objectEnumerator];
    wiimote = [i nextObject];
  }
  [delegate WiimoteDiscoveryResult:error wiimote:wiimote];
  state = IDLE;
}

@end



@implementation Wii

-(id) init
{
  self = [super init];
  if(self != nil){
    delegate = nil;
    state = DISCONNECTED;
    opened = NO;
    device = nil;
    iChan = nil;
    cChan = nil;
    timer = nil;
    disconnectNote = nil;
    connected_channels = 0;
    discovery = [[WiiDiscover alloc] init];
    if(discovery != nil){
      [discovery setDelegate:self];
    }else{
      [self release];
      self = nil;
    }
  }
  return self;
}

-(void) dealloc
{
  [discovery setDelegate:nil];
  [discovery release];
  discovery = nil;
  if(iChan != nil){
    [iChan release];
  }
  iChan = nil;
  if(cChan != nil){
    [cChan release];
  }
  cChan = nil;
  [super dealloc];
}

-(id) delegate
{
  return delegate;
}

-(void) setDelegate:(id) _delegate
{
  delegate = _delegate;
}

-(void) wiiFSM:(event_t) event
{
  switch(state){
    case DISCONNECTED:
      if(event == CONNECT_REQUEST){
        state = CONNECTING;
        [discovery start];
      }
      break;
    case CONNECTING:
      if(event == CONNECT_EVENT){
        state = CONNECTED;
        timer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self
           selector:@selector(initializeWii:) userInfo:nil repeats:NO];
        [timer retain];
      }else if(event == DISCONNECT_REQUEST){
        NSLog(@"Have DISCONNECT REQUEST in state CONNECTING!");
        [delegate wiiDisconnected];
        state = DISCONNECTING;
        [self close];
      }
      break;
    case CONNECTED:
      if((event == DISCONNECT_REQUEST) || (event == DISCONNECT_EVENT)){
        NSLog(@"Have DISCONNECT REQUEST in state CONNECTED!");
        [delegate wiiDisconnected];
        state = DISCONNECTING;
        [self close];
      }
      break;
    case DISCONNECTING:
      if(event == DISCONNECT_EVENT){
        state = DISCONNECTED;
      }else if(event == DISCONNECT_REQUEST){
        NSLog(@"Have DISCONNECT REQUEST in state DISCONNECTING!");
        [self close];
      }
      break;
    default:
      NSLog(@"UNKNOWN STATE!!!!");
      break;
  }
}

-(void) WiimoteDiscoveryResult:(IOReturn) error wiimote:(id) wiimote
{
  if(error == kIOReturnSuccess){
    device = wiimote;
    [self connect];
  }
}


-(void) connect
{
  if((state != CONNECTING)&&(state !=CONNECTED)){
    NSLog(@"Called connect in bad state!");
    return;
  }
  IOReturn res;
  if(!opened){
    usleep(100000);
    opened = YES;
//    disconnectNote = [device registerForDisconnectNotification:self selector:@selector(disconnected:fromDevice:)];
//    if(disconnectNote == nil){
//      LogIOReturn(res);
//      NSLog(@"Couldn't register for disconnect notification!");
//      [self wiiFSM:DISCONNECT_REQUEST];
//    }
  }

  if(cChan == nil){
    NSLog(@"Opening control channel...");
    res = [device openL2CAPChannelSync:&cChan 
             withPSM:kBluetoothL2CAPPSMHIDControl delegate:self];
    if(res != kIOReturnSuccess){
      LogIOReturn(res);
      NSLog(@"Couldn't open control L2CAP channel!");
      [self wiiFSM:DISCONNECT_REQUEST];
    }
    [cChan retain];
    return;
  }

  if(iChan == nil){
    NSLog(@"Opening interrupt channel...");
    res = [device openL2CAPChannelSync:&iChan 
             withPSM:kBluetoothL2CAPPSMHIDInterrupt delegate:self];
    if(res != kIOReturnSuccess){
      LogIOReturn(res);
      NSLog(@"Couldn't open interrupt L2CAP channel!");
      [self wiiFSM:DISCONNECT_REQUEST];
    }
    [iChan retain];
    return;
  }
  
  if(connected_channels != 3){
    return;
  }
  [self wiiFSM:CONNECT_EVENT];
}

-(void) start
{
  NSLog(@"Start requested!");
  [self wiiFSM:CONNECT_REQUEST];
}

-(void) stop
{
  NSLog(@"Stop requested!");
  [self wiiFSM:DISCONNECT_REQUEST];
}

-(void) close
{
  NSLog(@"Closing!!!");
  if(disconnectNote != nil){
    [disconnectNote unregister];
    disconnectNote = nil;
  }
  if(iChan != nil){
    [iChan closeChannel];
    return;
  }
  if(cChan != nil){
    [cChan closeChannel];
    return;
  }
  if(opened){
    NSLog(@"Closing baseconnection!");
    [device closeConnection];
    opened = NO;
  }
  [self wiiFSM:DISCONNECT_EVENT];
  return;
}

-(void) disconnected:(IOBluetoothUserNotification*) note fromDevice:(IOBluetoothDevice*) _device
{
  (void) note; 
  NSLog(@"Disconnected something...");
  if(_device == device){
    NSLog(@"Disconnected!");
    [self wiiFSM:DISCONNECT_REQUEST];
  }
}

-(void) l2capChannelReconfigured:(IOBluetoothL2CAPChannel*) l2capChannel
{
  (void) l2capChannel;
}

-(void) l2capChannelWriteComplete:(IOBluetoothL2CAPChannel*) l2capChannel 
          refcon:(void*) refcon status:(IOReturn) error
{
  (void) l2capChannel;
  (void) refcon;
  (void) error;
}

-(void) l2capChannelQueueSpaceAvailable:(IOBluetoothL2CAPChannel*) l2capChannel
{
  (void) l2capChannel;
}

-(void) l2capChannelOpenComplete:(IOBluetoothL2CAPChannel*) l2capChannel status:(IOReturn) error
{
  if(error == kIOReturnSuccess){
    if(l2capChannel == cChan){
      connected_channels |= 1; 
      [self updateReportMode:NO];
      NSLog(@"Control Channel Opened!");
    }else if(l2capChannel == iChan){
      connected_channels |= 2; 
      NSLog(@"Interrupt Channel Opened!");
    }
    [self connect];
  }else{
    NSLog(@"Error opening channel!");
    LogIOReturn(error);
  }
}

-(void) l2capChannelClosed:(IOBluetoothL2CAPChannel*) l2capChannel
{
  if(l2capChannel == cChan){
    connected_channels &= ~1;
    cChan = nil;
    NSLog(@"Control Channel closed!");
  }else if(l2capChannel == iChan){
    connected_channels &= ~2;
    iChan = nil;
    NSLog(@"Interrupt Channel closed!");
  }else{
    NSLog(@"**ERROR** UNKNOWN CHANNEL!!!");
  }
  if((state == CONNECTING) || (state == CONNECTED)){
    NSLog(@"Still connected - trying to restore!");
    [self connect];
  }else{
    NSLog(@"Not connected anymore!");
    [self wiiFSM:DISCONNECT_REQUEST];
  }
} 

void convert_triplet(unsigned char *source, ir_data_t *data)
{
  data->x = (source[2] >> 4) & (int)0x03;
  data->x = (data->x << 8) + source[0];
  data->y = (source[2] >> 6) & (int)0x03;
  data->y = (data->y << 8) + source[1];
  data->size = source[2] & (int)0x0F;
}

-(void) decodeIRData:(unsigned char *)data len:(int) len
{
  (void) len;
  static ir_data_t ir_data[4];
  switch(data[1]){
    case 0x33:
/*      
      NSLog(@"%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", 
          data[7], data[8], data[9], 
          data[10], data[11], data[12], 
          data[13], data[14], data[15], 
          data[16], data[17], data[18]);
*/
      convert_triplet(&(data[7]), &(ir_data[0]));      
      convert_triplet(&(data[10]), &(ir_data[1]));      
      convert_triplet(&(data[13]), &(ir_data[2]));      
      convert_triplet(&(data[16]), &(ir_data[3]));      
      
      [delegate wiiIRData:ir_data];
      break;
    case 0x36:
      NSLog(@"%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", 
          data[4], data[5], data[6], data[7], data[8], data[9], 
          data[10], data[11], data[12], data[13]);
      break;
    case 0x37:
      NSLog(@"%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", 
          data[7], data[8], data[9], data[10], data[11], data[12], 
          data[13], data[14], data[15], data[16]);
      break;
  }
}

-(void) l2capChannelData:(IOBluetoothL2CAPChannel*) l2capChannel data:(void *) dataPointer length:(size_t) dataLength
{
  (void) l2capChannel;
  if(state != CONNECTED){
    NSLog(@"Got some data while not connected!");
    return;
  }
  unsigned char *ptr = (unsigned char *)dataPointer;
  if((ptr[1] == 0x33) && (dataLength == 19)){
    [self decodeIRData:ptr len:dataLength];
  }else if(ptr[1] == 0x36){
    [self decodeIRData:ptr len:dataLength];
  }else if(ptr[1] == 0x37){
    [self decodeIRData:ptr len:dataLength];
  }else if(ptr[1] == 0x20){
    NSLog(@"Buttons: %02X%02X", ptr[2] & 0x1F, ptr[3] & 0x9F);
    NSLog(@"Battery: %d", ptr[7]);
    NSLog(@"Flags: %02X", ptr[4]);
    [self updateReportMode:irOn];
  }
  static int counter = 0;
  if(++counter > 1000){
    counter = 0;
    [self updateReportMode:irOn];
  }
}

-(void) updateReportMode:(BOOL) ir_on
{
  // Core buttons
  unsigned char cmd[] = {0x12, 0x02, 0x30};
  if(ir_on){ //Core buttons, accelerometer and extended IR data
    cmd[2] |= 0x03;
  }
  [self sendCommand:cmd length:3];
}

-(IOReturn) sendCommand:(const unsigned char *) data length:(size_t) length
{
  unsigned char buf[40];
  if(state != CONNECTED){
    return kIOReturnSuccess;
  }
  //NSLog(@"Sending command!");
  memset(buf, 0, 40);

  buf[0] = 0x52;
  memcpy(buf+1, data, length);
  if(buf[1] == 0x16){
    length = 23;
  }else{
    length++;
  }

  IOReturn ret = kIOReturnSuccess;
  
  ret = [cChan writeSync:buf length:length];    
  if(ret != kIOReturnSuccess) {
    NSLog(@"Write Error for command 0x%x: 0x%X", buf[1], ret);    
    LogIOReturn(ret);
  }
  return ret;
}

-(IOReturn) writeData:(const unsigned char*) data at:(unsigned long) address length:(size_t) length
{
  unsigned char cmd [22];

  if (length > 16){
    NSLog (@"Error! Trying to write more than 16 bytes of data (length=%i)", length);
    return kIOReturnError;
  }
  memset (cmd, 0, 22);
  memcpy (cmd + 6, data, length);

  // register write header
  cmd[0] = 0x16;
  
  // write address
  cmd[1] = (address >> 24) & 0xFF;
  cmd[2] = (address >> 16) & 0xFF;
  cmd[3] = (address >>  8) & 0xFF;
  cmd[4] = (address >>  0) & 0xFF;
  
  // data length
  cmd[5] = length;
  return [self sendCommand:cmd length:22];
}


-(void) setLEDEnabled:(int) enabled
{
  //NSLog(@"Enabling LEDs...");
  unsigned char cmd[] = {0x11, 0x00};
//  if (vibrationEnabled)  cmd[1] |= 0x01;
  if(enabled & 1)  cmd[1] |= 0x10;
  if(enabled & 2)  cmd[1] |= 0x20;
  if(enabled & 4)  cmd[1] |= 0x40;
  if(enabled & 8)  cmd[1] |= 0x80;
  
  IOReturn ret = [self sendCommand:cmd length:2];
  if(ret != kIOReturnSuccess){
    NSLog(@"Problem sending command! 0x%X");
  }
}

-(void) enableIR
{
  NSLog(@"Enabling IR Sensor");
  IOReturn ret = kIOReturnSuccess;
  unsigned char cmd[] = {0x13, 0x04};
  ret = [self sendCommand:cmd length:2];
  LogIOReturn(ret);
  usleep(60000);
  
  unsigned char cmd2[] = {0x1a, 0x04};
  ret = [self sendCommand:cmd2 length:2];
  LogIOReturn(ret);
  usleep(60000);
  
  // start initializing camera
  ret = [self writeData:(darr){0x08} at:0x04B00030 length:1];
  LogIOReturn(ret);
  usleep(60000);
  
  // set sensitivity block 1
  ret = [self writeData:(darr){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x41} at:0x04B00000 length:9];
  LogIOReturn(ret);
  usleep(60000);
  
  // set sensitivity block 2
  ret = [self writeData:(darr){0x40, 0x00} at:0x04B0001A length:2];
  LogIOReturn(ret);
  usleep(60000);
      
  // set mode number (Extended)
  ret = [self writeData:(darr){0x03} at:0x04B00033 length:1];
  LogIOReturn(ret);
  usleep(60000);
  
  // finish initializing camera
  ret = [self writeData:(darr){0x08} at:0x04B00030 length:1];
  LogIOReturn(ret);
  usleep(60000);
  
  if (ret != kIOReturnSuccess) {
    NSLog(@"Set IR Enabled failed, closing connection");
    return;
  }
  
  irOn = YES;
  [self updateReportMode:YES];
}

-(void) disableIR
{
  irOn = NO;
  NSLog(@"Disabling IR Sensor");
  IOReturn ret = kIOReturnSuccess;
  unsigned char cmd[] = {0x13, 0x00};
  ret = [self sendCommand:cmd length:2];
  LogIOReturn(ret);
  usleep(60000);
  
  unsigned char cmd2[] = {0x1a, 0x00};
  ret = [self sendCommand:cmd2 length:2];
  LogIOReturn(ret);
  usleep(60000);
  [self updateReportMode:NO];
}

-(void) initializeWii:(NSTimer*)theTimer
{
  (void)theTimer;
  [timer release];
  timer = nil;
  NSLog(@"Initializing Wiimote!");
  irOn = NO;
  unsigned char cmd[] = {0x15, 0x00};
  [self sendCommand:cmd length:2];
  [self disableIR];
  [self setLEDEnabled:0];
  [self updateReportMode:NO];
  [delegate wiiConnected];
}

-(void) deinitializeWii
{
  NSLog(@"Deinitializing Wiimote!");
  [self setLEDEnabled:0];
  [self disableIR];
}

-(void) pause:(int) leds
{
  [self disableIR];
  usleep(100000);
  [self setLEDEnabled:leds];
}

-(void) resume:(int) leds
{
  [self enableIR];
  usleep(100000);
  [self setLEDEnabled:leds];
}

@end
