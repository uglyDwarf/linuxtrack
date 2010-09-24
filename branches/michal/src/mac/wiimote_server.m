//
//  wiimote_server.m
//  WiiSrvr
//
//  Created by Michal Navratil on 8/30/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "wiimote_server.h"
#import "com_proc.h"
#import "wii_com.h"
#import "../image_process.h"
#import <string.h>

#define WIIMOTE_HORIZONTAL_RESOLUTION 1024
#define WIIMOTE_VERTICAL_RESOLUTION 768

static enum {WII_DISCONNECTED, WII_CONNECTING, WII_CONNECTED} server_state = WII_DISCONNECTED;

@implementation wiimote_server

- (IBAction) connectPressed:(id) sender
{
  (void) sender;
  switch(server_state){
    case WII_DISCONNECTED: 
      [status setStringValue:@"Connecting..."];
      server_state = WII_CONNECTING;
      [wiimote start];
      break;
    case WII_CONNECTED:
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
  static command_t old_cmd = STOP;
  command_t cmd;
  cmd = getCommand();
  if(cmd != old_cmd){
    switch(cmd){
      case STOP:
        [wiimote pause:0];
        break;
      case SLEEP:
        [wiimote pause:1];
        break;
      case WAKEUP:
        [wiimote resume:3];
        break;
    }
  }
  old_cmd = cmd;
}

-(void) wiiConnected
{
  server_state = WII_CONNECTED;
  [status setStringValue:@"Connected..."];
  [connect setTitle:@"Disconnect"];
  NSLog(@"We are connected to the wiimote!");
  [wiimote pause:0];
  timer = [NSTimer scheduledTimerWithTimeInterval:0.2 target:self 
           selector:@selector(timerCallback:) userInfo:nil repeats:YES];
  [timer retain];
}

-(void) wiiDisconnected
{
  server_state = WII_DISCONNECTED;
  [status setStringValue:@"Disconnected..."];
  [connect setTitle:@"Connect"];
  [timer invalidate];
  NSLog(@"We are disconnected!");
}

-(void) wiiIRData:(ir_data_t*)data
{
  int i;
  int valid = 0;
  bool get_frame = getFrameFlag();
  image img = {
    .w = WIIMOTE_HORIZONTAL_RESOLUTION / 2,
    .h = WIIMOTE_VERTICAL_RESOLUTION / 2,
    .bitmap = getFramePtr(),
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
      (blobs_array[valid]).x = data[i].x;
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
  setBlobs(blobs_array, bloblist.num_blobs);
  if(!get_frame){
    setFrameFlag();
  }

}

-(void) awakeFromNib
{
  if(!initWiiCom(true)){
    NSLog(@"Can't start server!");
    return;
  }
  wiimote = [[Wii alloc] init];
  [wiimote setDelegate:self];
  server_state = WII_DISCONNECTED;
  [status setStringValue:@"Disconnected..."];
  ltr_int_prepare_for_processing(WIIMOTE_HORIZONTAL_RESOLUTION, WIIMOTE_VERTICAL_RESOLUTION);
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
  closeWiiCom();
  [super dealloc];
}


@end
