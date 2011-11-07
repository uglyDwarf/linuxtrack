#include <stdio.h>
#include <com_proc.h>
#include <utils.h>

static char *contactFile = ".linuxtrack_wii";
static char *prefFile = ".linuxtrack_wii.lock";
static char *fullContactFile = NULL;
static char *fullPrefFile = NULL;
//static semaphore_p pfSem = NULL;

static struct mmap_s mmm;

/*
// -1 Error
//  0 Server not running
//  1 Server runs already
static int serverRunningAlready(bool should_lock)
{
  //Determine full name of the pref file
  fullPrefFile = ltr_int_get_default_file_name(prefFile);
  if(fullPrefFile == NULL){
    ltr_int_log_message("Can't determine pref file path!\n");
    return -1;
  }
  pfSem = ltr_int_createSemaphore(fullPrefFile);
  if(pfSem == NULL){
    ltr_int_log_message("Can't create semaphore!");
    return -1;
  }
  if(ltr_int_tryLockSemaphore(pfSem) == false){
    ltr_int_closeSemaphore(pfSem);
    pfSem= NULL;
    ltr_int_log_message("Can't lock - server runs already!\n");
    return 1;
  }
  
  
  return 0;
}
*/
bool ltr_int_initWiiCom(bool isServer, struct mmap_s **mmm_p)
{
  semaphore_p lock_sem = NULL;
  if(isServer){
    if(ltr_int_server_running_already(prefFile, &lock_sem, true) != 0){
      ltr_int_log_message("Server is already running!\n");
      return false;
    }
  }else{
    if(ltr_int_server_running_already(prefFile, NULL, false) != 1){
      ltr_int_log_message("Server not running!\n");
      return false;
    }
  }

  //Determine full name of the contact file
  fullContactFile = ltr_int_get_default_file_name(contactFile);
  if(fullContactFile == NULL){
    ltr_int_log_message("Can't determine contact file path!\n");
    return false;
  }
  if(!ltr_int_mmap_file(fullContactFile, ltr_int_get_com_size() + 512 * 384, &mmm)){
    ltr_int_log_message("Can't mmap comm file!\n");
    return false;
  }
  mmm.lock_sem = lock_sem;
  *mmm_p = &mmm;
  ltr_int_log_message("Wii com initialized @ %s!\n", fullContactFile);
  return true;
}

void ltr_int_closeWiiCom()
{
  ltr_int_setCommand(&mmm, STOP);
  ltr_int_unmap_file(&mmm);
  
  if(fullContactFile != NULL){
    free(fullContactFile);
    fullContactFile = NULL;
  }
  if(fullPrefFile != NULL){
    free(fullPrefFile);
    fullPrefFile = NULL;
  }
  ltr_int_log_message("Wii com Closed!\n");
}

void ltr_int_pauseWii()
{
  ltr_int_setCommand(&mmm, SLEEP);
  ltr_int_log_message("Wii com SLEEP!\n");
}

void ltr_int_resumeWii()
{
  ltr_int_setCommand(&mmm, WAKEUP);
  ltr_int_log_message("Wii com WAKEUP!\n");
}

