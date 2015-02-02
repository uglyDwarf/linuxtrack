#include "ltr_srv_comm.h"
#include "ltr_srv_slave.h"
#include "ltr_srv_master.h"
#include "utils.h"
#include <signal.h>

int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  ltr_int_check_root();
  //Make sure that broken pipe won't bring us down
  signal(SIGPIPE, SIG_IGN);
  if(argc == 1){
    ltr_int_master(true);
  }else{
    //Parameter is name of profile
    ltr_int_slave(argv[1], argv[2], argv[3]);
  }
  return 0;
}

