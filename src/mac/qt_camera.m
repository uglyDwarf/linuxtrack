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
#include <com_proc.h>
#include "processing.h"
#include <utils.h>



@interface cam : NSObject
{
    QTCaptureDeviceInput                *mCaptureDeviceInput;
    QTCaptureDevice                     *mCaptureDevice;
    QTCaptureSession                    *mCaptureSession;
    QTCaptureDecompressedVideoOutput    *mCaptureDecompressedVideoOutput;
}

-(id) initWebcam: (const char*)devName: (int)width : (int)height;
-(void)dealloc;
-(void)start;
-(void)stop;

+(QTCaptureDevice *) findCam:(const char*) devname;
+(void) enumerateWebcams;

@end


@implementation cam


static printWebcamName(FILE *f, const char *name)
{
  fprintf(f, "'%s' ", name);
  int i;
  for(i = 0; i < strlen(name); ++i){
    fprintf(f, "%02X ", (unsigned char)name[i]);
  }
  fprintf(f, "\n");
}

+(QTCaptureDevice *) findCam: (const char*)devname
{
  //[devices release];
  const char *name;
  size_t len = strlen(devname);
  FILE *wcList = NULL;
  const char *injectWebcamName = getenv("LINUXTRACK_WC_NAME");
  if(injectWebcamName != NULL){
    wcList = fopen(ltr_int_get_default_file_name("../../webcam_list.txt"), "w");
    printWebcamName(wcList, devname);
  }
  
  NSArray *devices = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo];
  for(QTCaptureDevice *dev in devices){
    name = [[dev localizedDisplayName] UTF8String];
    if(wcList != NULL){
      printWebcamName(wcList, name);
    }
    if(strncmp(devname, name, len) == 0){
      if(wcList != NULL){
        fclose(wcList);
      }
      return dev;
    }
  }
  if(wcList != NULL){
    fclose(wcList);
  }
  return nil;
}

-(id) initWebcam: (const char*)devName: (int)width : (int)height
{
  NSError *err = nil;
  if((self = [super init])){
    
    mCaptureDevice = [cam findCam:devName];
    if(mCaptureDevice == nil){
      fprintf(stderr, "Couldn't find webcam '%s'\n", 
        devName);
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
      [NSNumber numberWithInt:height], kCVPixelBufferHeightKey, 
      [NSNumber numberWithInt:width], kCVPixelBufferWidthKey, 
      [NSNumber numberWithUnsignedInt:kCVPixelFormatType_422YpCbCr8], (id)kCVPixelBufferPixelFormatTypeKey,
//      [NSNumber numberWithUnsignedInt:kCVPixelFormatType_32BGRA], (id)kCVPixelBufferPixelFormatTypeKey,
      nil]];
    [mCaptureDecompressedVideoOutput setAutomaticallyDropsLateVideoFrames:YES];
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
  const char *injectWebcamName = getenv("LINUXTRACK_WC_NAME");
  if((injectWebcamName != NULL)){
    size_t len = strlen(injectWebcamName);
    size_t i = 0;
    int tmp, skip;
    if(len > 0){
      char *name = (char *)malloc(len);
      i = 0;
      while(sscanf(injectWebcamName, "%02X%n", &tmp, &skip)>0){
        injectWebcamName += skip;
        name[i++] = tmp;
      }
      printf("%s\n", name);
      free(name);
    }
  }
  NSArray *devices = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo];
  //[devices release];
  for(QTCaptureDevice *dev in devices){
    printf("%s\n", [[dev localizedDisplayName] UTF8String]);
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

bool capture(struct mmap_s *mmm)
{
  int x, y;
  getRes(&x, &y);
  fprintf(stderr, "Capturing from '%s' @ %dx%d using '%s'!\n",
          getCamName(), x, y, getMapFileName());
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

  NSApplicationLoad();
  
  const char *cam_name = getCamName();
  
  cam *webcam = [[cam alloc] initWebcam:cam_name:x:y];
  if(webcam == NULL){
    fprintf(stderr, "Can't open device for capture!\n");
    [pool release];
    return false;
  }
  
  if(!startProcessing(x, y, 4, mmm)){
    fprintf(stderr, "Can't initialize processing!\n");
    return false;
  }  
  bool capturing = true;
  [webcam start];
  
  command_t cmd, old_cmd;
  old_cmd = WAKEUP;
  
  while((cmd = ltr_int_getCommand(mmm)) != STOP){
    if(old_cmd != cmd){
      ltr_int_printCmd("old", old_cmd);
      ltr_int_printCmd("new", cmd);
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
    }else{
      ltr_int_usleep(200000);
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
