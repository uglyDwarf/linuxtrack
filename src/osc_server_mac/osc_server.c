#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <lo/lo.h>
#include <linuxtrack.h>
#include <pthread.h>

static pthread_t tid;
static bool loopOn = true;

#define BLOBS 10

static linuxtrack_pose_t pose;
static float blobs[BLOBS*3];
static int blobsRead;

static lo_address t;

//True is b1 is "bigger" than b2
bool blobCompare(int b1, int b2)
{
  //Compare Y values
  if(blobs[3*b1+1] < blobs[3*b2+1]){
    return true;
  }else if(blobs[3*b1+1] == blobs[3*b2+1]){
    //To make Murphy stop laughing...
    //  When Y equals, sort by X
    if(blobs[3*b1] < blobs[3*b2]){
      return true;
    }
  }
  return false;
}

void swapBlobs(int b1, int b2)
{
  float x, y, w;
  x = blobs[3*b1];
  y = blobs[3*b1+1];
  w = blobs[3*b1+2];

  blobs[3*b1] = blobs[3*b2];
  blobs[3*b1+1] = blobs[3*b2+1];
  blobs[3*b1+2] = blobs[3*b2+2];

  blobs[3*b2] = x;
  blobs[3*b2+1] = y;
  blobs[3*b2+2] = w;
}

//Bubble sort; should be enough for this amount of
void blobSort(int blobs)
{
  bool changed = true;
  int i;
  int swaps = 0;
  int passes = 0;
  do{
    ++passes;
    changed = false;
    for(i = 0; i < blobs-1; ++i){
      if(blobCompare(i, i+1)){
        swapBlobs(i, i+1);
        changed = true;
        ++swaps;
      }
    }
  }while(changed);
  //printf("Sort: %d passes, %d swaps\n", passes, swaps);
}

bool intialize_tracking(void)
{
	linuxtrack_state_type state;
	//Initialize the tracking using Default profile
	state = linuxtrack_init(NULL);
	if(state < LINUXTRACK_OK){
		printf("%s\n", linuxtrack_explain(state));
		return false;
	}
	int timeout = 0;
	//wait up to 20 seconds for the tracker initialization
	while(timeout < 200){
		state = linuxtrack_get_tracking_state();
		printf("Status: %s\n", linuxtrack_explain(state));
		if((state == RUNNING) || (state == PAUSED)){
			linuxtrack_notification_on();
			return true;
		}
		usleep(1000000);
		++timeout;
	}
	printf("Linuxtrack doesn't work right!\n");
	printf("Make sure it is installed and configured correctly.\n");
	return false;
}


bool sendPose(lo_address addr)
{
	lo_bundle bundle = lo_bundle_new(LO_TT_IMMEDIATE);
	lo_message msg1 = lo_message_new();
	
	
	
	//  lo_message_add(msg1, "sfsfsfsfsfsf", "pitch", pose.pitch, "yaw", pose.yaw, "roll", pose.roll,
	//                                           "X", pose.tx, "Y", pose.ty, "Z", pose.tz);
	lo_message_add(msg1, "ffffff", pose.pitch, pose.yaw, pose.roll, pose.tx, pose.ty, pose.tz);
	lo_bundle_add_message(bundle, "/linuxtrack/pose", msg1);
	
	int i;
	for(i = 0; i < blobsRead; ++i){
		lo_message msg2 = lo_message_new();
		//    lo_message_add(msg2, "sisfsfsf", "index", i, "x", blobs[3*i], "y", blobs[3*i+1], "weight", blobs[3*i+2]);
		lo_message_add(msg2, "ifff", i, blobs[3*i], blobs[3*i+1], blobs[3*i+2]);
		lo_bundle_add_message(bundle, "/linuxtrack/point", msg2);
	}
	
	lo_send_bundle(addr, bundle);
	lo_bundle_free_recursive(bundle);
	return true;
}


void *cmdThread(void *param)
{
  (void) param;
  if(!intialize_tracking()){
    return NULL;
  }
  while(loopOn){
    if(linuxtrack_get_pose_full(&pose, blobs, BLOBS, &blobsRead) > 0){
      blobSort(blobsRead);
      sendPose(t);
    }
    linuxtrack_wait(3333);
  }
  linuxtrack_shutdown();
  return NULL;
}

void stopTracking()
{
  loopOn = false;
}


bool oscServerInit(int port)
{
	printf("Starting at port %d\n", port);
  char portString[10];
  if(port <= 1024){
    printf("Please select port number higher than 1024.\n");
    return false;
  }
  snprintf(portString, sizeof(portString), "%d", port);
  t = lo_address_new(NULL, portString);
  loopOn = true;
  pthread_create(&tid, NULL, cmdThread, NULL);
  return true;
}

