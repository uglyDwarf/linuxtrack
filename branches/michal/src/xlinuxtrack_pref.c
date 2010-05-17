#include "xlinuxtrack_pref.h"
#include <assert.h>

struct pref{
  int start_stop;
  int pause;
} pref;

static char *pref_file = ".xplaneltr";

char *get_pref_file_name()
{
  char *home = getenv("HOME");
  if(home == NULL){
    log_message("Please set HOME variable!\n");
    return NULL;
  }
  char *pref_path = (char *)my_malloc(strlen(home) 
                    + strlen(pref_file) + 2);
  sprintf(pref_path, "%s/%s", home, pref_file);
  return pref_path;
}


void print_pref(struct pref *p)
{
  assert(p != NULL);
  printf("Start/Stop %d\n", get_pref(p, START_STOP));
  printf("Pause %d\n", get_pref(p, PAUSE));
}

bool read_pref(char *fname, struct pref *p)
{
  assert(fname != NULL);
  assert(p != NULL);
  char id[1024] = "";
  int val = -1;
  int res;
  FILE *f = fopen(fname, "r");
  
  reset_pref(p);
  
  if(f == NULL){
    return false;
  }
  
  while(!feof(f)){
    res = fscanf(f, "%1000s %d", id, &val);
    if(strcasecmp(id, "start/stop")){
      set_pref(p, START_STOP, val);
    }
    if(strcasecmp(id, "pause")){
      set_pref(p, PAUSE, val);
    }
  }
  fclose(f);
  
  return is_pref_valid(p);
}

bool save_pref(char *fname, struct pref *p)
{
  assert(fname != NULL);
  assert(p != NULL);
  FILE *f = fopen(fname, "w");
  
  if(f == NULL){
    return false;
  }
  
  if(fprintf(f, "Start/Stop	%d\n", get_pref(p, START_STOP)) < 0){
    return false;
  }
  
  if(fprintf(f, "Pause		%d\n", get_pref(p, PAUSE)) < 0){
    return false;
  }
  fflush(f);
  fclose(f);
  return true;
}

void reset_pref(struct pref *p)
{
  assert(p != NULL);
  p->start_stop = -1;
  p->pause = -1;
}

bool is_pref_valid(struct pref *p)
{
  assert(p != NULL);
  if((p->start_stop == -1) || (p->pause == -1)){
    return false;
  }
  return true;
}

bool set_pref(struct pref *p, enum pref_id id, int val)
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

int get_pref(struct pref *p, enum pref_id id)
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
      return -1;
      break;
  }
}

struct pref *new_pref()
{
  struct pref *p = my_malloc(sizeof(struct pref));
  reset_pref(p);
  return p;
}
