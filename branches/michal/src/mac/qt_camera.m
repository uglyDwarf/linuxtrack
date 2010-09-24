/*
  TODO: Why I don't specify YUV data I need?
 */

#import <Cocoa/Cocoa.h>
#import <Foundation/NSObject.h>
#import <QTKit/QTKit.h>
#import <QTKit/QTCaptureDevice.h>
#import <CoreVideo/CVPixelBuffer.h>

#import <unistd.h>
#include <stdio.h>
#include "args.h"
#include "com_proc.h"
#include "processing.h"



@interface cam : NSObject
{
    QTCaptureDeviceInput                *mCaptureDeviceInput;
    QTCaptureDevice                     *mCaptureDevice;
    QTCaptureSession                    *mCaptureSession;
    QTCaptureDecompressedVideoOutput    *mCaptureDecompressedVideoOutput;
}

-(id) initWebcam: (NSString*)devName: (int)width : (int)height;
-(void)dealloc;
-(void)start;
-(void)stop;

+(QTCaptureDevice *) findCam:(NSString*) devname;
+(void) enumerateWebcams;

@end


@implementation cam

+(QTCaptureDevice *) findCam: (NSString*)devname
{
  NSArray *devices = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo];
  //[devices release];
  for(QTCaptureDevice *dev in devices){
    NSString *name = [dev localizedDisplayName];
    if([name caseInsensitiveCompare:devname] == NSOrderedSame){
      return dev;
    }
  }
  return nil;
}

-(id) initWebcam: (NSString*)devName: (int)width : (int)height
{
  NSError *err = nil;
  if((self = [super init])){
    mCaptureDevice = [cam findCam:devName];
    if(mCaptureDevice == nil){
      fprintf(stderr, "Couldn't find webcam '%s'\n", 
        [devName cStringUsingEncoding:NSStringEncodingConversionExternalRepresentation]);
      [self release];
      return nil;
    }
    if(![mCaptureDevice open:&err]){
      fprintf(stderr, "Problem opening webcam '%s'!\n", 
        [[err localizedDescription] cStringUsingEncoding:NSStringEncodingConversionExternalRepresentation]);
      [self release];
      return nil;
    }
    mCaptureDeviceInput = [QTCaptureDeviceInput deviceInputWithDevice:mCaptureDevice];
    [mCaptureDeviceInput retain];
    if(mCaptureDeviceInput == nil){
      fprintf(stderr, "Problem creating input device!\n");
      [self release];
      return nil;
    }
    mCaptureSession = [[QTCaptureSession alloc] init];
    if(mCaptureSession == nil){
      fprintf(stderr, "Problem creating session!\n");
      [self release];
      return nil;
    }
    if(![mCaptureSession addInput:mCaptureDeviceInput error:&err]){
      fprintf(stderr, "Problem adding input to the session!\n");
      [self release];
      return nil;
    }
    mCaptureDecompressedVideoOutput = [[QTCaptureDecompressedVideoOutput alloc] init];
    if(mCaptureDecompressedVideoOutput == nil){
      fprintf(stderr, "Problem creating output!\n");
      [self release];
      return nil;
    }
    [mCaptureDecompressedVideoOutput setDelegate:self];
    [mCaptureDecompressedVideoOutput setPixelBufferAttributes:[NSDictionary dictionaryWithObjectsAndKeys:
      [NSNumber numberWithInt:height], kCVPixelBufferHeightKey, [NSNumber numberWithInt:width], kCVPixelBufferWidthKey, nil]];
    if(![mCaptureSession addOutput:mCaptureDecompressedVideoOutput error:&err]){
      fprintf(stderr, "Problem adding output!\n");
      [self release];
      return nil;
    }
  }
  return self;
}

-(void)start
{
  [mCaptureSession startRunning];
}

-(void)stop
{
  [mCaptureSession stopRunning];
}


// This delegate method is called whenever the QTCaptureDecompressedVideoOutput receives a frame
- (void)captureOutput:(QTCaptureOutput *)captureOutput didOutputVideoFrame:(CVImageBufferRef)videoFrame withSampleBuffer:(QTSampleBuffer *)sampleBuffer fromConnection:(QTCaptureConnection *)connection
{
  (void) captureOutput;
  (void) sampleBuffer;
  (void) connection;
  static int counter = 0;
  CVPixelBufferRef frame = (CVPixelBufferRef)videoFrame;
//  printf("Have Frame No. %d (%lu x %lu, %lu bytes, %lu bytes pre row)!\n", counter, CVPixelBufferGetWidth(frame), 
//    CVPixelBufferGetHeight(frame), CVPixelBufferGetDataSize(frame), CVPixelBufferGetBytesPerRow(frame));
  static char *buffer = nil;
  size_t pixels = CVPixelBufferGetWidth(frame) * CVPixelBufferGetHeight(frame);
  if(buffer == nil){
    buffer = (char *)malloc(pixels);
  }
  CVPixelBufferLockBaseAddress(frame, 0);
  unsigned char *ptr = CVPixelBufferGetBaseAddress(frame);
  newFrame(ptr);

  CVPixelBufferUnlockBaseAddress(frame, 0);
//  printf("Finished frame %d\n", ++counter);
}


- (void)dealloc
{
  [mCaptureSession stopRunning];
  if(mCaptureDecompressedVideoOutput != nil){
    [mCaptureDecompressedVideoOutput release];
    mCaptureDecompressedVideoOutput = nil;
  }
  if(mCaptureSession != nil){
    [mCaptureSession release];
    mCaptureSession = nil;
  }
  if(mCaptureDeviceInput != nil){
    [mCaptureDeviceInput release];
    mCaptureDeviceInput = nil;
  }
  if([mCaptureDevice isOpen]){
    [mCaptureDevice close];
    [mCaptureDevice release];
    mCaptureDevice = nil;
  }
  [super dealloc];
}

+(void) enumerateWebcams
{
  NSArray *devices = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo];
  //[devices release];
  for(QTCaptureDevice *dev in devices){
    printf("%s\n", [[dev localizedDisplayName] cStringUsingEncoding:NSStringEncodingConversionExternalRepresentation]);
  }
}

@end

void enumerateCameras()
{
  fprintf(stderr, "Enumerating qt_cam!\n");
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
  [cam enumerateWebcams];
  [pool release];
  return;
}

bool capture()
{
  int x, y;
  getRes(&x, &y);
  fprintf(stderr, "Capturing from '%s' @ %dx%d using '%s'!\n",
          getCamName(), x, y, getMapFileName());
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

  NSApplicationLoad();
  
  NSString *cam_name = [[NSString alloc] initWithCString:getCamName() encoding:NSASCIIStringEncoding];
  
  cam *webcam = [[cam alloc] initWebcam:cam_name:x:y];
  if(webcam == NULL){
    fprintf(stderr, "Can't open device for capture!\n");
    [pool release];
    return false;
  }
  
  startProcessing(x, y, 4);  
  bool capturing = true;
  [webcam start];
  
  command_t cmd, old_cmd;
  old_cmd = WAKEUP;
  
  while((cmd = getCommand()) != STOP){
    if(old_cmd != cmd){
      printCmd("old", old_cmd);
      printCmd("new", cmd);
      printf("\n");
    }
    switch(old_cmd){
      case STOP:
      case SLEEP:
	if(cmd == WAKEUP){
	  capturing = true;
          [webcam start];
	}
	break;
      case WAKEUP:
	if(cmd == SLEEP){
	  capturing = false;
          [webcam stop];
	}
	break;
      default:
	break;
    }
    if(capturing){
      [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
    }
    old_cmd = cmd;
  }
  if(capturing){
    [webcam stop];
  }
  endProcessing();
  [webcam dealloc];
  [pool release];
  
  return true;
}
