#include <stdio.h>
#include <stdlib.h>

#include "args.h"
#include "com_proc.h"
#include "../ipc_utils.h"
#include "mac_camera.h"

struct mmap_s mmm;

int main(int argc, char *argv[])
{
  if(!checkCmdLine(argc, argv)){
    fprintf(stderr, "Wrong  arguments!\n");
    return EXIT_FAILURE;
  }
  
  if(doEnumCams()){
    enumerateCameras();
  }else if(doCapture()){
    int x, y;
    getRes(&x, &y);
    // Double buffer...
    if(mmap_file(getMapFileName(), get_com_size() + x * y, &mmm)){
      setCommand(&mmm, WAKEUP);
      capture(&mmm);
      unmap_file(&mmm);
    }else{
      fprintf(stderr, "Can't mmap!\n");
      return EXIT_FAILURE;
    }
  }else{
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}
