#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <lo/lo.h>
#include <linuxtrack.h>
#include <pthread.h>

#include <config.h>

static pthread_t tid;
static bool loopOn = true;

#define BLOBS 10

static linuxtrack_pose_t pose;
static float blobs[BLOBS*3];
static int blobsRead;

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


void *cmdThread(void *param)
{
  (void) param;
  int c;
  while(loopOn){
    printf("1) Pause tracking\n");
    printf("2) Resume tracking\n");
    printf("3) Recenter tracking\n");
    printf("4) Tracking status\n");
    printf("5) Quit tracking\n");
    printf("Your choice (1-5): ");
    do{
      c = getchar();
    }while(c == '\n');
    printf("\n");
    switch(c){
      case '1':
        linuxtrack_suspend();
        break;
      case '2':
        linuxtrack_wakeup();
        break;
      case '3':
        linuxtrack_recenter();
        break;
      case '4':
        printf("Status: %s\n", linuxtrack_explain(linuxtrack_get_tracking_state()));
        break;
      case '5':
        loopOn = false;
        break;
      default:
        printf("Please select one of choices (1-5) and press enter.\n");
        break;
    }
  }
  return NULL;
}


/*
  if (lo_send(t, "/linuxtrack/pose", "sfsfsfsfsfsf", "pitch", 35.0f, "yaw", 22.0f, "roll", 10.0f,
                                           "X", 5.2f, "Y", -6.56, "Z", 444.3f) == -1) {
    printf("OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t));
  }

 */


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
  //Available on newer versions only
#ifdef HAVE_NEW_LIBLO
  lo_bundle_free_recursive(bundle);
#else
  lo_bundle_free_messages(bundle);
#endif
  return true;
}

int main(int argc, char *argv[])
{
  if(argc < 2){
    printf("Please provide a port number as an argument.\n");
    return 0;
  }
  int port = atoi(argv[1]);
  char portString[10];
  if(port <= 1024){
    printf("Please select port number higher than 1024.\n");
    return 0;
  }
  snprintf(portString, sizeof(portString), "%d", port);
  lo_address t = lo_address_new(NULL, portString);
    if (argc > 2 && argv[2][0] == '-' && argv[2][1] == 'q') {
        printf("Sending quit msg.\n");
        /* send a message with no arguments to the path /quit */
        if (lo_send(t, "/quit", NULL) == -1) {
            printf("OSC error %d: %s\n", lo_address_errno(t),
                   lo_address_errstr(t));
        }
    } else {
        /* send a message to /foo/bar with two float arguments, report any
         * errors */
      if(!intialize_tracking()){
        return 0;
      }
      pthread_create(&tid, NULL, cmdThread, NULL);
      while(loopOn){
        if(linuxtrack_get_pose_full(&pose, blobs, BLOBS, &blobsRead) > 0){
          blobSort(blobsRead);
          sendPose(t);
        }
        linuxtrack_wait(3333);
      }
    }

    linuxtrack_shutdown();
    return 0;
}

