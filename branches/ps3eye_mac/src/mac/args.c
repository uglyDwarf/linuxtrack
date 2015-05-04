#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "args.h"

static bool enum_cams_flag = false;
static bool capture = false;
static bool map = false;
static bool facetrack = false;
static char *cam_id = NULL;
static char *map_file = NULL;
static char *cascade_file = NULL;
static int w = -1;
static int h = -1;

bool doEnumCams()
{
  return enum_cams_flag;
}

bool doCapture()
{
  return capture;
}

bool doMap()
{
  return map;
}

const char *getCamName()
{
  return cam_id;
}

const char *getMapFileName()
{
  return map_file;
}

const char *getCascadeFileName()
{
  return cascade_file;
}

bool doFacetrack()
{
  return facetrack;
}

void getRes(int *x, int *y)
{
  *x = w;
  *y = h;
}

bool checkCmdLine(int argc, char *argv[])
{
  int ch;
  while((ch = getopt(argc, argv, "ec:x:y:f:d:")) != -1){
    switch(ch){
      case 'e':
	enum_cams_flag = true;
	break;
      case 'c':
	if((cam_id = strdup(optarg)) != NULL){
	  capture = true;
	}
	break;
      case 'x':
	w = atoi(optarg);
	break;
      case 'y':
	h = atoi(optarg);
	break;
      case 'f':
	  if((map_file = strdup(optarg)) != NULL){
	    map = true;
	  }
	break;
      case 'd':
	  if((cascade_file = strdup(optarg)) != NULL){
	    facetrack = true;
	  }
	break;
    }
  }
  return enum_cams_flag || (capture && map && (w != -1) && (h != -1));
}
