#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <utils.h>
#include <ipc_utils.h>
#include <ltlib_int.h>




// Safety - if parent dies, we should follow
void *safety_thread(void *param)
{
  (void)param;
  printf("Safety thread started!\n");
  while(1){
    if(getppid() == 1){
      printf("Parent died!\n");
      //check the state and try shutdown first...
      //Spawn third one with timed exit, in case of HW fubar (deadlock,...)?
      exit(0);
    }
    sleep(1);
  }
}

pthread_t st;

void start_safety()
{
  pthread_create(&st, NULL, safety_thread, NULL);
}

static char *lockName = "ltr_server.lock";
static semaphore_p pfSem = NULL;

void new_frame(struct frame_type *frame, void *param)
{
  (void)frame;
  struct mmap_s *mmm = (struct mmap_s*)param;
  struct ltr_comm *com = mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  ltr_int_get_camera_update(&(com->heading), &(com->pitch), &(com->roll), 
                            &(com->tx), &(com->ty), &(com->tz), &(com->counter));
  ltr_int_unlockSemaphore(mmm->sem);
}

void state_changed(void *param)
{
  struct mmap_s *mmm = (struct mmap_s*)param;
  struct ltr_comm *com = mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  com->state = ltr_int_get_tracking_state();
  ltr_int_unlockSemaphore(mmm->sem);
}

void main_loop(char *section)
{
  bool recenter;

  char *com_file = ltr_int_get_com_file_name();
  struct mmap_s mmm;
  if(!ltr_int_mmap_file(com_file, sizeof(struct ltr_comm), &mmm)){
    printf("Couldn't mmap file!!!\n");
    return;
  }
  
  ltr_int_register_cbk(new_frame, (void*)&mmm, state_changed, (void*)&mmm);
  if(ltr_int_init(section) != 0){
    printf("Not initialized!\n");
    ltr_int_unmap_file(&mmm);
    return;
  }
  struct ltr_comm *com = mmm.data;
  bool break_flag = false;
  while(!break_flag){
    if(com->cmd != NOP_CMD){
      ltr_int_lockSemaphore(mmm.sem);
      ltr_cmd cmd = com->cmd;
      com->cmd = NOP_CMD;
      recenter = com->recenter;
      com->recenter = false;
      ltr_int_unlockSemaphore(mmm.sem);
      switch(cmd){
        case RUN_CMD:
          ltr_int_wakeup();
          break;
        case PAUSE_CMD:
          ltr_int_suspend();
          break;
        case STOP_CMD:
          ltr_int_shutdown();
          break_flag = true;
          break;
        default:
          //defensive...
          break;
      }
    }
    if(recenter){
      ltr_int_recenter();
    }
    usleep(100000);  //ten times per second...
  }
  ltr_int_unmap_file(&mmm);
}

int main(int argc, char *argv[])
{
  char *section = NULL;

  if(argc > 1){
    section = argv[1];
    //Section name
  }
  ltr_int_set_logfile_name("/tmp/ltr_server.log");
  
  start_safety();
  pfSem = ltr_int_server_running_already(lockName);
  if(pfSem != NULL){
    main_loop(section);
    ltr_int_closeSemaphore(pfSem);
  }
  printf("Finishing server!\n");
  return 0;
}

