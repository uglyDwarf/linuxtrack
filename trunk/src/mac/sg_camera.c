#include <stdio.h>
#include <unistd.h>
#include <QuickTime/QuickTimeComponents.h>
#include <Carbon/Carbon.h>

#include "args.h"
#include <com_proc.h>
#include "processing.h"

#define CHECK_RES(x, str)\
{\
  if(x != noErr){\
    fprintf(stderr, "%s: %s\n", str,\
    GetMacOSStatusErrorString(result));\
    return false;\
  }\
}

/*
#define PIXFMT k24RGBPixelFormat
#define FILE_ROW_FACTOR 3
#define ROW_FACTOR 3
#define SKIP 1
#define OFFSET 0

#define PIXFMT k32ARGBPixelFormat
#define FILE_ROW_FACTOR 4
#define ROW_FACTOR 4
#define SKIP 1
#define OFFSET 0

#define PIXFMT kYUVSPixelFormat
#define FILE_ROW_FACTOR 1
#define ROW_FACTOR 2
#define SKIP 2
#define OFFSET 0
*/
#define PIXFMT kYUVSPixelFormat
#define FILE_ROW_FACTOR 1
#define ROW_FACTOR 2
#define SKIP 2
#define OFFSET 0

static SeqGrabComponent grabber = NULL;
static SGChannel channel = NULL;
static GWorldPtr gworld = NULL;
static unsigned char *pixBuffer = NULL;
static ImageSequence sequence;
static Rect window;

bool initChannel()
{
  OSErr result = noErr;
  grabber = OpenDefaultComponent(SeqGrabComponentType, 0);
  if(!grabber){
    fprintf(stderr, "No grabber!\n");
    return false;
  }
  result = SGInitialize(grabber);
  if(result != noErr){
    fprintf(stderr, "Error init sg...\n");
    return false;
  }
  result = SGSetDataRef(grabber, 0, 0, seqGrabToMemory | seqGrabDontMakeMovie);
  if(result != noErr){
    fprintf(stderr, "Error setting data ref...\n");
    return false;
  }
  result = SGNewChannel(grabber, VideoMediaType, &channel);
  if(result != noErr){
    fprintf(stderr, "Error setting new channel... %s\n", 
    GetMacOSStatusErrorString(result));
    return false;
  }
  return true;
}

void closeChannel()
{
  if(channel != NULL){
    SGDisposeChannel(grabber, channel);
  }
  if(grabber != NULL){
    CloseComponent(grabber);
  }
}

int checkDevs()
{
  OSErr result = noErr;
  
  if(!initChannel()){
    closeChannel();
    return 1;
  }
  
  SGDeviceList dev_list = 0;
  result = SGGetChannelDeviceList(channel, sgDeviceListIncludeInputs, &dev_list);
  if(result == noErr){
    int i, j;
    for(i = 0; i < ((*dev_list)->count); ++i){
      SGDeviceName device = (*dev_list)->entry[i];
      SGDeviceInputList device_input_list = device.inputs;
      if(device_input_list != NULL){
        for(j = 0; j < ((*device_input_list)->count); ++j){
          SGDeviceInputName input_name_rec = (*device_input_list)->entry[j];
          printf("%s\n", input_name_rec.name+1);
        }
      }
    }
  }
  return 0;
}



bool openDev(const char *name)
{
  OSErr result = noErr;
  EnterMovies();
  
  if(!initChannel()){
    closeChannel();
    return 1;
  }
  
  SGDeviceList dev_list = 0;
  result = SGGetChannelDeviceList(channel, sgDeviceListIncludeInputs, &dev_list);
  if(result != noErr){
    fprintf(stderr, "Error getting channel device list... %s\n", 
    GetMacOSStatusErrorString(result));
    return false;
  }
  int i, j;
  for(i = 0; i < ((*dev_list)->count); ++i){
    SGDeviceName device = (*dev_list)->entry[i];
    SGDeviceInputList device_input_list = device.inputs;
    if(device_input_list != NULL){
      for(j = 0; j < ((*device_input_list)->count); ++j){
        SGDeviceInputName input_name_rec = (*device_input_list)->entry[j];
        if(strcasecmp(name, (char *)input_name_rec.name+1) == 0){
          result = SGSetChannelDeviceInput(channel, j);
          if(result != noErr){
            fprintf(stderr, "Error setting channel input... %s\n", 
            GetMacOSStatusErrorString(result));
            return false;
          }else{
            return true;
          }
        }
      }
    }
  }
  return false;
}

void enumerateCameras()
{
  fprintf(stderr, "sg_cam: Enumerating...\n");
  checkDevs();
  return;
}

OSErr captureCallback(SGChannel c, Ptr p, long len, long *offset, long chRefCon, 
		      TimeValue time, short writeType, long refCon)
{
  (void) c;
  (void) p;
  (void) len;
  (void) offset;
  (void) chRefCon;
  (void) time;
  (void) writeType;
  (void) refCon;
  
  OSErr result = noErr;
  static bool firstTime = true;
  
  if(firstTime){
    firstTime = false;
    ImageDescriptionHandle desc = (ImageDescriptionHandle)NewHandle(0);
    result = SGGetChannelSampleDescription(channel, (Handle)desc);
    if(result != noErr){
      fprintf(stderr, "SGGetChannelSampleDescription: %s\n", 
              GetMacOSStatusErrorString(result));
      return noErr;
    }
    MatrixRecord scaleMatrix;
    RectMatrix(&scaleMatrix,&window,&window);
    result = DecompressSequenceBeginS(&sequence, desc, p, len, gworld, NULL, &window, &scaleMatrix,
				      srcCopy, NULL, 0, codecNormalQuality, bestSpeedCodec);
    if(result != noErr){
      fprintf(stderr, "DecompressSequenceBeginS: %s\n", 
              GetMacOSStatusErrorString(result));
      return noErr;
    }
    DisposeHandle((Handle) desc);
  }else{
    CodecFlags ignore;
    result = DecompressSequenceFrameS(sequence, p, len, 0, &ignore, NULL);
    if(result != noErr){
      fprintf(stderr, "DecompressSequenceFrameS: %s\n", 
              GetMacOSStatusErrorString(result));
      return noErr;
    }
  }
  newFrame(pixBuffer);
  return noErr;
}

bool setCaptureParams(int w, int h)
{
  OSErr result = noErr;
  window.right = w;
  window.left = 0;
  window.bottom = h;
  window.top = 0;
  pixBuffer = (unsigned char *)malloc(w * h * ROW_FACTOR);
  result = QTNewGWorldFromPtr(&gworld, PIXFMT, &window, 0, NULL, 0, pixBuffer, ROW_FACTOR * w);
  CHECK_RES(result, "QTNewGWorld")
  result = SGSetGWorld(grabber, gworld, NULL);
  result = SGSetChannelBounds(channel, &window);
  CHECK_RES(result, "SGSetChannelBounds")
  result = SGSetChannelUsage(channel, seqGrabRecord);
  CHECK_RES(result, "SGSetChannelUsage")
  result = SGSetDataProc(grabber, captureCallback,0);
  CHECK_RES(result, "SGSetDataProc")
  return true;
}

bool capture(struct mmap_s *mmm)
{
  OSErr result = noErr;
  int x, y;
  getRes(&x, &y);
  fprintf(stderr, "Capturing from '%s' @ %dx%d using '%s'!\n",
          getCamName(), x, y, getMapFileName());
  if(!openDev(getCamName())){
    fprintf(stderr, "Can't open device for capture!\n");
    closeChannel();
    return false;
  }
  if(!setCaptureParams(x, y)){
    fprintf(stderr, "Can't set capture parameters!\n");
    closeChannel();
    return false;
  }
  bool capturing = true;
  result = SGStartRecord(grabber);
  CHECK_RES(result, "SGStartRecord")
  
  command_t cmd, old_cmd;
  old_cmd = WAKEUP;
  
  startProcessing(x, y, 4, mmm);  
  while((cmd = ltr_int_getCommand(mmm)) != STOP){
    if(old_cmd != cmd){
      ltr_int_printCmd("old", old_cmd);
      ltr_int_printCmd("new", cmd);
      printf("\n");
    }
    switch(old_cmd){
      case STOP:
      case SLEEP:
	if(cmd == WAKEUP){
	  capturing = true;
	  SGPause(grabber, seqGrabUnpause);
          CHECK_RES(result, "SGStartRecord")
	}
	break;
      case WAKEUP:
	if(cmd == SLEEP){
	  capturing = false;
	  SGPause(grabber, seqGrabPause);
	}
	break;
      default:
	break;
    }
    if(capturing){
      result = SGIdle(grabber);
      CHECK_RES(result, "SGIdle")
    }
    ltr_int_usleep(20000);
    old_cmd = cmd;
  }
  if(capturing){
    SGStop(grabber);
  }
  closeChannel();
  endProcessing();
  return true;
}
