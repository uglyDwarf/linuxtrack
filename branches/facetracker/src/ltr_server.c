#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <utils.h>
#include <ipc_utils.h>
#include <ltlib_int.h>
#include <pref_global.h>
#include <ltr_server.h>

static bool dead_man_button_pressed = false;
static bool all_clients_gone = false;
static bool active = false;
static struct mmap_s mmm;

// Safety - if parent dies, we should follow
static void *safety_thread(void *param)
{
  (void)param;
  int counter = 0;
  while(1){
    sleep(1);
    struct ltr_comm *com = mmm.data;
    //PID 1 means INIT process, and it means our parent process died.
    if(getppid() != 1){
      if(com != NULL){
        com->dead_man_button = true;
      }else{
        ltr_int_log_message("Mmap channel not initialized properly!\n");
        exit(1);
        break; //mmm.data is NULL, something went wrong!
      }
    }else{
      if(!active) break;
    }
    
    if(active){
      if(dead_man_button_pressed){
        dead_man_button_pressed = false;
        counter = 0;
      }else{
        if(counter > 60){
          ltr_int_log_message("No response for too long, exiting...\n");
          all_clients_gone = true;
          break;
        }
        ++counter;
      }
    }
  }
  return NULL;
}

static pthread_t st;

static void start_safety(bool act)
{
  active = act;
  pthread_create(&st, NULL, safety_thread, NULL);
  if(!active){
    pthread_join(st, NULL);
  }
}

static char *lockName = "ltr_server.lock";
static semaphore_p pfSem = NULL;

#ifndef LTR_GUI
static void new_frame(struct frame_type *frame, void *param)
{
  (void)frame;
  struct mmap_s *mmm = (struct mmap_s*)param;
  struct ltr_comm *com = mmm->data;
  ltr_int_lockSemaphore(mmm->sem);
  ltr_int_get_camera_update(&(com->heading), &(com->pitch), &(com->roll), 
                            &(com->tx), &(com->ty), &(com->tz), &(com->counter));
  ltr_int_unlockSemaphore(mmm->sem);
}
#endif

static void state_changed(void *param)
{
  struct mmap_s *mmm = (struct mmap_s*)param;
  struct ltr_comm *com = (struct ltr_comm*)mmm->data;
  if(mmm->data != NULL){
    ltr_int_lockSemaphore(mmm->sem);
    com->state = ltr_int_get_tracking_state();
    ltr_int_unlockSemaphore(mmm->sem);
  }
}

static bool ltr_int_is_inactive(ltr_state_type state)
{
  return((state == STOPPED) || (state == ERROR));
}

static void main_loop(char *section)
{
  bool recenter = false;
  
  ltr_int_register_cbk(new_frame, (void*)&mmm, state_changed, (void*)&mmm);

  struct ltr_comm *com = mmm.data;
  ltr_int_lockSemaphore(mmm.sem);
  com->cmd = NOP_CMD;
  com->recenter = true;
  com->state = STOPPED;
  com->heading = 0.0f;
  com->pitch = 0.0f;
  com->roll = 0.0f;
  com->tx = 0.0f;
  com->ty = 0.0f;
  com->tz = 0.0f;
  com->counter = 0;
  com->dead_man_button = 0;
  com->preparing_start = 1;
  ltr_int_unlockSemaphore(mmm.sem);
  
  
  //Do the init step
  if(ltr_int_init(section) != 0){
    ltr_int_log_message("Not initialized!\n");
    ltr_int_unmap_file(&mmm);
    return;
  }

  while(1){
    switch(com->state){
      case STOPPED:
      case INITIALIZING:
        //Tracker is initializing, wait for the initialization finish
        break;
      case RUNNING:
      case PAUSED:
        dead_man_button_pressed |= com->dead_man_button;
        com->dead_man_button = false;
        com->preparing_start = false;
        if((com->cmd != NOP_CMD) || com->recenter){
          ltr_int_lockSemaphore(mmm.sem);
          ltr_cmd cmd = (ltr_cmd)com->cmd;
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
              //we handle stopping through dead man's button
              // - we start shutdown countdown when all our clients die;
              // If some new client pops up, we start again
              ltr_int_suspend();
#ifdef LTR_GUI
              goto shutdown;
#endif
              break;
            case NOP_CMD:
              break;
            default:
              //defensive...
              assert(0);
              break;
          }
        }
        break;
      case ERROR:
        goto shutdown;
        break;
      default:
        assert(0);
        break;
    }
    if(recenter){
      recenter = false;
      ltr_int_recenter();
    }
#ifndef LTR_GUI
    if(all_clients_gone){ // here the shutdown is handled...
      goto shutdown;
    }
#endif
    ltr_int_usleep(100000);  //ten times per second...
  }

 shutdown:
  //shutdown and wait till it is down (with timeout).
  ltr_int_shutdown();
  int timeout_counter = 30;
  while(!ltr_int_is_inactive(com->state)){
    ltr_int_usleep(100000);  //ten times per second...
    if(--timeout_counter <= 0){
      ltr_int_log_message("Wait for shutdown timed out, exiting anyway...\n");
      com->state = ERROR; // Just in case
      break;
    }
  }
#ifndef LTR_GUI
  ltr_int_close_prefs();
#endif
}

int prep_main_loop(char *section)
{
  char *com_file = ltr_int_get_com_file_name();
  if(!ltr_int_mmap_file(com_file, sizeof(struct ltr_comm), &mmm)){
    printf("Couldn't mmap file!!!\n");
    return -1;
  }
  free(com_file);
  int res = ltr_int_server_running_already(lockName, &pfSem, true);
  if(res == 0){
    ltr_int_log_message("Starting server in the active mode!\n");
#ifdef LTR_GUI
    ltr_init(section);
#endif
    start_safety(true);
    main_loop(section);
    ltr_int_log_message("Just left the main loop!\n");
    ltr_int_closeSemaphore(pfSem);
#ifdef LTR_GUI    
    pthread_cancel(st);
    pthread_join(st, NULL);
#endif
  }else{
#ifndef LTR_GUI
    ltr_int_log_message("Starting server in the passive mode!\n");
    start_safety(false);
#endif
  }
  ltr_int_unmap_file(&mmm);
  ltr_int_log_message("Finishing server!\n");
  return res;
}

#ifndef LTR_GUI

int main(int argc, char *argv[])
{
  char *section = NULL;

  if(argc > 1){
    section = argv[1];
    ltr_int_log_message("ltr_server - section '%s'\n", section);
    //Section name
  }else{
    ltr_int_log_message("ltr_server - section NULL\n");
  }
  return prep_main_loop(section);
}

#endif

