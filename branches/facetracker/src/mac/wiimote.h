#import <Cocoa/Cocoa.h>
#import <IOBluetooth/objc/IOBluetoothDeviceInquiry.h> 
#import <IOBluetooth/objc/IOBluetoothDevice.h>
#import <IOBluetooth/objc/IOBluetoothL2CAPChannel.h>

@interface WiiDiscover : NSObject 
{
  IOBluetoothDeviceInquiry *inquiry;
  id delegate;
  enum {IDLE, REQUEST_DISCOVERY, DISCOVERING} state;
}

-(id) init;
-(void) dealloc;
-(id) delegate;
-(void) setDelegate:(id) _delegate;
-(IOReturn) start;

//IOBluetoothDeviceInquiry delegates
-(void) deviceInquiryStarted:(IOBluetoothDeviceInquiry*) sender;
-(void) deviceInquiryDeviceFound:(IOBluetoothDeviceInquiry*) sender
          device:(IOBluetoothDevice*) device;
-(void) deviceInquiryComplete:(IOBluetoothDeviceInquiry*) sender
          error:(IOReturn) error  aborted:(BOOL) aborted;


@end

@interface NSObject (WiiDiscoverDelegate)
  -(void) WiimoteDiscoveryResult:(IOReturn) error wiimote:(id) wiimote;
@end

@interface Wii : NSObject 
{
  WiiDiscover *discovery;
  IOBluetoothDevice *device;
  BOOL opened;
  BOOL irOn;
  IOBluetoothL2CAPChannel *iChan;
  IOBluetoothL2CAPChannel *cChan;
  IOBluetoothUserNotification *disconnectNote;
  id delegate;
  NSTimer *timer;
  enum {DISCONNECTED, CONNECTING, CONNECTED, DISCONNECTING} state;
  int connected_channels;
}
typedef enum {CONNECT_REQUEST, CONNECT_EVENT, DISCONNECT_REQUEST,
      DISCONNECT_EVENT} event_t;

-(id) init;
-(void) dealloc;
-(id) delegate;
-(void) setDelegate:(id) _delegate;
-(void) wiiFSM:(event_t) event;
-(void) connect;
-(void) close;
-(void) disconnected:(IOBluetoothUserNotification*) note fromDevice:(IOBluetoothDevice*) _device;
-(void) l2capChannelReconfigured:(IOBluetoothL2CAPChannel*) l2capChannel;
-(void) l2capChannelWriteComplete:(IOBluetoothL2CAPChannel*) l2capChannel refcon:(void*) refcon status:(IOReturn) error;
-(void) l2capChannelQueueSpaceAvailable:(IOBluetoothL2CAPChannel*) l2capChannel;
-(void) l2capChannelOpenComplete:(IOBluetoothL2CAPChannel*) l2capChannel status:(IOReturn) error;
-(void) l2capChannelData:(IOBluetoothL2CAPChannel*) l2capChannel data:(void *) dataPointer length:(size_t) dataLength;
-(void) l2capChannelClosed:(IOBluetoothL2CAPChannel*) l2capChannel;
-(void) initializeWii:(NSTimer*)theTimer;
-(void) deinitializeWii;
-(void) updateReportMode:(BOOL) ir_on;
-(IOReturn) sendCommand:(const unsigned char *) data length:(size_t) length;
-(IOReturn) writeData:(const unsigned char*) data at:(unsigned long) address length:(size_t) length;
-(void) setLEDEnabled:(int) enabled;
-(void) enableIR;
-(void) disableIR;
-(void) decodeIRData:(unsigned char *)data len:(int) len;
-(void) start;
-(void) stop;
-(void) pause:(int) leds;
-(void) resume:(int) leds;

-(void) WiimoteDiscoveryResult:(IOReturn) error wiimote:(id) wiimote;
@end

typedef struct {
  int x, y, size;
} ir_data_t;


@interface NSObject (WiiDelegate)
  -(void) wiiConnected;
  -(void) wiiIRData:(ir_data_t*)data;
  -(void) wiiDisconnected;
@end
