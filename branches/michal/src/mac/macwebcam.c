#include <stdio.h>
#include <stdlib.h>

#include "args.h"
#include "com_proc.h"
#include "mac_camera.h"

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
    if(mmap_file(getMapFileName(), x * y)){
      setCommand(WAKEUP);
      capture();
      unmap_file();
    }else{
      fprintf(stderr, "Can't mmap!\n");
      return EXIT_FAILURE;
    }
  }else{
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}
