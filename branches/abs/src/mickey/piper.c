#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include "../sn4_com.h"
#include "../utils.h"
#include "../ipc_utils.h"

static semaphore_p lock_sem = NULL;

int prepareBtnChanel()
{
  //printf("Opening the channel!\n");
  signal(SIGPIPE, SIG_IGN);
  char *fname = ltr_int_get_default_file_name("ltr_sn4.pipe");
  int fifo = ltr_int_open_fifo_exclusive(fname, &lock_sem);
  free(fname);
  if(fifo <= 0){
    //printf("Can't open fifo!\n");
    return -1;
  }
  //printf("Fifo opened!\n");
  return fifo;
}

int closeBtnChannel(int fifo)
{
  close(fifo);
  ltr_int_unlockSemaphore(lock_sem);
  ltr_int_closeSemaphore(lock_sem);
  return 0;
}

bool fetch_data(int fifo, void *data, ssize_t length, ssize_t *read)
{
  assert(data != NULL);
  assert(read != NULL);
  *read = 0;
  int res = ltr_int_pipe_poll(fifo, 1000, NULL);
  switch(res){
    case 0://Timeout
      return true;
      break;
    case 1://have data!
      //intentionaly do nothing!
      break;
    default:
      return false;
      break;
  }
  *read = ltr_int_fifo_receive(fifo, data, length);
  return true;
}


/*
int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  sn4_btn_event_t ev;
  int fifo = prepareBtnChanel();
  ssize_t read;
  while(fetch_data(fifo, (void*)&ev, sizeof(ev), &read)){
    if(read == sizeof(ev)){
      printf("Piper received %ld\n", read);
      printf("%d\n", ev.btns);
    }
  }
  return 0;
}
*/
