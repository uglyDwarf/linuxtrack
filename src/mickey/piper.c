#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include "../sn4_com.h"
#include "../utils.h"
#include "../ipc_utils.h"

int prepareBtnChanel()
{
  //printf("Opening the channel!\n");
  signal(SIGPIPE, SIG_IGN);
  char *fname = ltr_int_get_default_file_name("ltr_sn4.socket");
  int socket = ltr_int_connect_to_socket(fname);
  free(fname);
  if(socket <= 0){
    //printf("Can't open socket!\n");
    return -1;
  }
  //printf("Socket opened!\n");
  return socket;
}

int closeBtnChannel(int socket)
{
  ltr_int_close_socket(socket);
  return 0;
}

bool fetch_data(int socket, void *data, ssize_t length, ssize_t *read)
{
  assert(data != NULL);
  assert(read != NULL);
  *read = 0;
  int res = ltr_int_pipe_poll(socket, 1000, NULL);
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
  *read = ltr_int_socket_receive(socket, data, length);
  return true;
}


/*
int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  sn4_btn_event_t ev;
  int socket = prepareBtnChanel();
  ssize_t read;
  while(fetch_data(socket, (void*)&ev, sizeof(ev), &read)){
    if(read == sizeof(ev)){
      printf("Piper received %ld\n", read);
      printf("%d\n", ev.btns);
    }
  }
  return 0;
}
*/
