#include "ltr_srv_comm.h"
#include "ltr_srv_slave.h"
#include "ltr_srv_master.h"

#include <signal.h>

int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  //Make sure that broken pipe won't bring us down
  signal(SIGPIPE, SIG_IGN);
  if(argc == 1){
    master(true);
  }else{
    //Parameter is name of profile
    slave(argv[1], argv[2], false);
  }
  return 0;
}

