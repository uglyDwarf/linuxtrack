#include <stdio.h>
#include "com_proc.h"
#include "../utils.h"

static char *contactFile = ".linuxtrack_wii";
static char *prefFile = ".linuxtrack_wii.lock";
static char *fullContactFile = NULL;
static char *fullPrefFile = NULL;
static semaphore_p pfSem = NULL;

static struct mmap_s mmm;

// -1 Error
//  0 Server not running
//  1 Server runs already
static int serverRunningAlready()
{
  //Determine full name of the pref file
  fullPrefFile = ltr_int_get_default_file_name(prefFile);
  if(fullPrefFile == NULL){
    ltr_int_log_message("Can't determine pref file path!\n");
    return -1;
  }
  pfSem = createSemaphore(fullPrefFile);
  if(pfSem == NULL){
    ltr_int_log_message("Can't create semaphore!");
    return -1;
  }
  if(tryLockSemaphore(pfSem) == false){
    closeSemaphore(pfSem);
    pfSem= NULL;
    ltr_int_log_message("Can't lock - server runs already!\n");
    return 1;
  }
  return 0;
}

bool initWiiCom(bool isServer, struct mmap_s **mmm_p)
{
  if(isServer){
    if(serverRunningAlready() != 0){
      ltr_int_log_message("Server is already running!\n");
      return false;
    }
  }else{
    if(serverRunningAlready() != 1){
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
  if(!mmap_file(fullContactFile, get_com_size() + 512 * 384, &mmm)){
    ltr_int_log_message("Can't mmap comm file!\n");
    return false;
  }
  *mmm_p = &mmm;
  ltr_int_log_message("Wii com initialized @ %s!\n", fullContactFile);
  return true;
}

void closeWiiCom()
{
  setCommand(&mmm, STOP);
  unmap_file(&mmm);
  if(pfSem != NULL){
    closeSemaphore(pfSem);
    pfSem= NULL;
  }
  
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

void pauseWii()
{
  setCommand(&mmm, SLEEP);
  ltr_int_log_message("Wii com SLEEP!\n");
}

void resumeWii()
{
  setCommand(&mmm, WAKEUP);
  ltr_int_log_message("Wii com WAKEUP!\n");
}

