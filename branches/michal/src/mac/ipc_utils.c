#define __USE_GNU
#include <unistd.h>
#undef __USE_GNU
#include <errno.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

static pid_t child;

bool fork_child(int in, int out, char *args[])
{
  (void) in;
  (void) out;
  if((child = fork()) == 0){
    //Child here
    execv(args[0], args);
    perror("execv");
    return false;
  }
  return true;
}

bool wait_child_exit(int limit)
{
  pid_t res;
  int status;
  int cntr = 0;
  do{
//    printf("Waiting!\n");
    usleep(100000);
    res = waitpid(child, &status, WNOHANG);
    ++cntr;
  }while((res != child) && (cntr < limit));
  if((res == child) && (WIFEXITED(status))){
    return true;
  }else{
    return false;
  }
}

