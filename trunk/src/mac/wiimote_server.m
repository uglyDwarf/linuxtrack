//
//  wiimote_server.m
//  WiiSrvr
//
//  Created by Michal Navratil on 8/30/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "wiimote_server.h"
#import <com_proc.h>
#import <wii_com.h>
#import <image_process.h>
#import <utils.h>
#import <ipc_utils.h>
#import <string.h>

#define WIIMOTE_HORIZONTAL_RESOLUTION 1024
#define WIIMOTE_VERTICAL_RESOLUTION 768

static struct mmap_s *mmm;
static command_t old_cmd = STOP;

static enum {WII_DISCONNECTED, WII_CONNECTING, WII_CONNECTED} server_state = WII_DISCONNECTED;

@implementation wiimote_server

- (IBAction) connectPressed:(id) sender
{
  (void) sender;
  switch(server_state){
    case WII_DISCONNECTED: 
      [status setTextColor:[NSColor orangeColor]];
      [status setStringValue:@"Connecting..."];
      server_state = WII_CONNECTING;
      [wiimote start];
      break;
    case WII_CONNECTED:
      [status setTextColor:[NSColor orangeColor]];
      [status setStringValue:@"Disconnecting..."];
      [timer invalidate];
      [wiimote stop];
      break;
    default:
      break;
  }
}

-(void) timerCallback:(NSTimer*)theTimer
{
  (void) theTimer;
  indication = ltr_int_getWiiIndication(mmm);
  command_t cmd;
  cmd = ltr_int_getCommand(mmm);
  if(cmd != old_cmd){
    switch(cmd){
      case STOP:
        [wiimote pause:0];
        indicate = YES;
        break;
      case SLEEP:
        indicate = NO;
        [wiimote pause:(indication & 15)];
        ltr_int_log_message("Pausing indication: %X\n", indication);
        break;
      case WAKEUP:
        indicate = NO;
        ltr_int_log_message("Waking indication: %X\n", indication);
        [wiimote resume:((indication >> 4) & 15)];
        break;
    }
  }
  old_cmd = cmd;
}


-(void) connectedIndicationOnCallback:(NSTimer*)theTimer
{
  if(!indicate){
    return;
  }
  static int st = 1;
  
  (void) theTimer;
  [wiimote setLEDEnabled:st];
  st <<= 1; 
  if(st > 8){
    st = 1;
  }
  connIndicatorOffTimer = [NSTimer scheduledTimerWithTimeInterval:0.05 target:self 
           selector:@selector(connectedIndicationOffCallback:) userInfo:nil repeats:NO];
  [connIndicatorOffTimer retain];
}

-(void) connectedIndicationOffCallback:(NSTimer*)theTimer
{
  [wiimote setLEDEnabled:0];
  [connIndicatorOffTimer release];
  connIndicatorOffTimer = nil;
}

-(void) wiiConnected
{
  server_state = WII_CONNECTED;
  [status setTextColor:[NSColor greenColor]];
  [status setStringValue:@"Connected..."];
  [connect setTitle:@"Disconnect"];
  NSLog(@"We are connected to the wiimote!");
  [wiimote pause:0];
  timer = [NSTimer scheduledTimerWithTimeInterval:0.2 target:self 
           selector:@selector(timerCallback:) userInfo:nil repeats:YES];
  [timer retain];
  connIndicatorOnTimer = [NSTimer scheduledTimerWithTimeInterval:5.0 target:self 
           selector:@selector(connectedIndicationOnCallback:) userInfo:nil repeats:YES];
  [connIndicatorOnTimer retain];
  indicate = YES;
}

-(void) wiiDisconnected
{
  indicate = NO;
  server_state = WII_DISCONNECTED;
  [status setTextColor:[NSColor redColor]];
  [status setStringValue:@"Disconnected..."];
  [connect setTitle:@"Connect"];
  [timer invalidate];
  timer = nil;
  [connIndicatorOnTimer invalidate];
  connIndicatorOnTimer = nil;
  old_cmd = STOP;
  NSLog(@"We are disconnected!");
}

-(void) wiiIRData:(ir_data_t*)data
{
  int i;
  int valid = 0;
  bool get_frame = ltr_int_getFrameFlag(mmm);
  image_t img = {
    .w = WIIMOTE_HORIZONTAL_RESOLUTION / 2,
    .h = WIIMOTE_VERTICAL_RESOLUTION / 2,
    .bitmap = ltr_int_getFramePtr(mmm),
    .ratio = 1.0
  };
  struct blob_type blobs_array[3] = {
    {0.0f, 0.0f, -1},
    {0.0f, 0.0f, -1},
    {0.0f, 0.0f, -1}
  };
  struct bloblist_type bloblist = {
    .num_blobs = 3,
    .blobs = blobs_array
  };
    
  if(!get_frame){
    memset(img.bitmap, 0, img.w * img.h);
  }
  
  for(i = 0; i < 4; ++i){
    if((data[i].size != 15) && (valid < 3)){
      (blobs_array[valid]).x = - data[i].x;
      (blobs_array[valid]).y = data[i].y;
      (blobs_array[valid]).score = data[i].size;
      if(!get_frame){
        ltr_int_draw_square(&img, data[i].x / 2, (WIIMOTE_VERTICAL_RESOLUTION - data[i].y) / 2,
          2*data[i].size);
        ltr_int_draw_cross(&img, data[i].x / 2, (WIIMOTE_VERTICAL_RESOLUTION - data[i].y) / 2, 
          (int)WIIMOTE_HORIZONTAL_RESOLUTION/100.0);
      }
      //NSLog(@"Point: [%d, %d], size %d", data[i].x, data[i].y, data[i].size);
      ++valid;
    }
  }
  bloblist.num_blobs = valid;
  ltr_int_setBlobs(mmm, blobs_array, bloblist.num_blobs);
  if(!get_frame){
    ltr_int_setFrameFlag(mmm);
  }
}

-(void) awakeFromNib
{
  if(!ltr_int_initWiiCom(true, &mmm)){
    NSLog(@"Can't start server!");
    return;
  }
  NSLog(@"Server communication initialized!");
  wiimote = [[Wii alloc] init];
  [wiimote setDelegate:self];
  server_state = WII_DISCONNECTED;
  [status setTextColor:[NSColor redColor]];
  [status setStringValue:@"Disconnected..."];
  ltr_int_prepare_for_processing(WIIMOTE_HORIZONTAL_RESOLUTION, WIIMOTE_VERTICAL_RESOLUTION);
  indicate = NO;
}

-(void) dealloc
{
  NSLog(@"Dealloc called!");
  if(timer != nil){
    [timer invalidate];
    [timer release];
    timer = nil;
  }
  if(wiimote != nil){
    [wiimote pause:0];
    [wiimote release];
  }
  ltr_int_closeWiiCom();
  [super dealloc];
}


@end
