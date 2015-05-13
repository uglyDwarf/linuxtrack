#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <ipc_utils.h>
#include <ltr_srv_comm.h>

int main(int argc, char *argv[]){
  (void) argc;
  (void) argv;

  //redirect stderr to /dev/null to avoid console spamming
  fflush(stderr);
  int n = open("/dev/null", O_WRONLY);
  if(n > 0){
    dup2(n, 2);
    close(n);
  }

  if(!ltr_int_is_fifo_locked(ltr_int_master_fifo_name())){
    //printf("Server not running!\n");
    return 1;
  }
  
  if(ltr_init(NULL) < LINUXTRACK_OK){
    //printf("Can't initialize communication.\n");
    return 1;
  }
  ltr_recenter();
  //printf("Recentered.\n");
  return 0;
}