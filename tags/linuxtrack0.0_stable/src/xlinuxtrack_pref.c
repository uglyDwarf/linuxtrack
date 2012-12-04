#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif
#include <stdio.h>

#include "xlinuxtrack_pref.h"
#include <linuxtrack.h>
#include <assert.h>
#include <errno.h>

struct pref{
  int start_stop;
  int pause;
} pref;

static char *pref_file = ".xplaneltr";

void* my_malloc(size_t size)
{
  void *ptr = malloc(size);
  if(ptr == NULL){
    ltr_log_message("Can't malloc memory! %s\n", strerror(errno));
    assert(0);
    exit(1);
  }
  return ptr;
}

char *xltr_get_pref_file_name()
{
  char *home = getenv("HOME");
  if(home == NULL){
    ltr_log_message("Please set HOME variable!\n");
    return NULL;
  }
  char *pref_path = NULL;
  asprintf(&pref_path, "%s/%s", home, pref_file);
  return pref_path;
}


void xltr_print_pref(struct pref *p)
{
  assert(p != NULL);
  printf("Start/Stop %d\n", xltr_get_pref(p, START_STOP));
  printf("Pause %d\n", xltr_get_pref(p, PAUSE));
}

bool xltr_read_pref(char *fname, struct pref *p)
{
  assert(fname != NULL);
  assert(p != NULL);
  char id[1024] = "";
  int val = -1;
  int res;
  FILE *f = fopen(fname, "r");
  
  xltr_reset_pref(p);
  
  if(f == NULL){
    return false;
  }
  
  while(!feof(f)){
    res = fscanf(f, "%1000s %d", id, &val);
    if(res < 0){
      continue;
    }
    if(strcasecmp(id, "start/stop")){
      xltr_set_pref(p, START_STOP, val);
    }
    if(strcasecmp(id, "pause")){
      xltr_set_pref(p, PAUSE, val);
    }
  }
  fclose(f);
  
  return xltr_is_pref_valid(p);
}

bool xltr_save_pref(char *fname, struct pref *p)
{
  assert(fname != NULL);
  assert(p != NULL);
  FILE *f = fopen(fname, "w");
  
  if(f == NULL){
    return false;
  }
  
  if(fprintf(f, "Start/Stop	%d\n", xltr_get_pref(p, START_STOP)) < 0){
    return false;
  }
  
  if(fprintf(f, "Pause		%d\n", xltr_get_pref(p, PAUSE)) < 0){
    return false;
  }
  fflush(f);
  fclose(f);
  return true;
}

void xltr_reset_pref(struct pref *p)
{
  assert(p != NULL);
  p->start_stop = -1;
  p->pause = -1;
}

bool xltr_is_pref_valid(struct pref *p)
{
  assert(p != NULL);
  if((p->start_stop == -1) || (p->pause == -1)){
    return false;
  }
  return true;
}

bool xltr_set_pref(struct pref *p, enum pref_id id, int val)
{
  assert(p != NULL);
  switch(id){
    case START_STOP:
      p->start_stop = val;
      break;
    case PAUSE:
      p->pause = val;
      break;
    default:
      return false;
      break;
  }
  return true;
}

int xltr_get_pref(struct pref *p, enum pref_id id)
{
  assert(p != NULL);
  switch(id){
    case START_STOP:
      return p->start_stop;
      break;
    case PAUSE:
      return p->pause;
      break;
    default:
      ltr_log_message("XLinuxtrack_pref: wrong pref ID: %d\n", id);
      return -1;
      break;
  }
}

struct pref *xltr_new_pref()
{
  struct pref *p = my_malloc(sizeof(struct pref));
  xltr_reset_pref(p);
  return p;
}
