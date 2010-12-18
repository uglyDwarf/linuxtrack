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


void new_frame(void *param)
{
  struct mmap_s *mmm = (struct mmap_s*)param;
  struct ltr_comm *com = mmm->data;
  lockSemaphore(mmm->sem);
  ltr_int_get_camera_update(&(com->heading), &(com->pitch), &(com->roll), 
                            &(com->tx), &(com->ty), &(com->tz), &(com->counter));
  unlockSemaphore(mmm->sem);
}

void state_changed(void *param)
{
  struct mmap_s *mmm = (struct mmap_s*)param;
  struct ltr_comm *com = mmm->data;
  lockSemaphore(mmm->sem);
  com->state = ltr_int_get_tracking_state();
  unlockSemaphore(mmm->sem);
}

char *section = NULL;

void main_loop(struct mmap_s *mmm)
{
  ltr_int_register_cbk(new_frame, (void*)mmm, state_changed, (void*)mmm);
  if(ltr_int_init(section) != 0){
    printf("Initialized!\n");
    return;
  }
  struct ltr_comm *com = mmm->data;
  bool break_flag = false;
  while(!break_flag){
    if(com->cmd != NOP_CMD){
      lockSemaphore(mmm->sem);
      ltr_cmd cmd = com->cmd;
      com->cmd = NOP_CMD;
      unlockSemaphore(mmm->sem);
      switch(cmd){
        case RUN_CMD:
          printf("Run!\n");
          ltr_int_wakeup();
          break;
        case PAUSE_CMD:
          printf("Pause!\n");
          ltr_int_suspend();
          break;
        case STOP_CMD:
          printf("Stop!\n");
          ltr_int_shutdown();
          break_flag = true;
          break;
        case RECENTER_CMD:
          printf("Recenter!\n");
          ltr_int_recenter();
          break;
        default:
          printf("SHIT!!!\n");
          //defensive...
          break;
      }
    }
    usleep(100000);  //ten times per second...
  }
}

int main(int argc, char *argv[])
{
  if(argc < 2){
    printf("I need filename!\n");
    return 1;
  }
  if(argc > 2){
    section = argv[2];
    //Comm file and section name
  }
  
  start_safety();
  struct mmap_s mmm;
  if(mmap_file(argv[1], sizeof(struct ltr_comm), &mmm)){
    printf("Have mmap!\n");
    main_loop(&mmm);
    unmap_file(&mmm);
  }
  printf("Finishing client!\n");
  return 0;
}

