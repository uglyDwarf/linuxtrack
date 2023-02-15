#include <string.h>
#include <stdlib.h>
#include "pref.h"
#include "pref_global.h"

static void decode(char *val, bool *d1, bool *d2, bool *d3, bool *d4)
{
  *d1 = (val[0] == '1')? true : false;
  *d2 = (val[1] == '1')? true : false;
  *d3 = (val[2] == '1')? true : false;
  *d4 = (val[3] == '1')? true : false;
}

static void encode(char *val, bool d1, bool d2, bool d3, bool d4)
{
  val[0] = d1 ? '1' : '0';
  val[1] = d2 ? '1' : '0';
  val[2] = d3 ? '1' : '0';
  val[3] = d4 ? '1' : '0';
  val[4] = '\0';
}


static char run_id_key[] = "Running-indication";
static char pause_id_key[] = "Paused-indication";
static char run_indication[6] = "0000";
static char pause_indication[6] = "0000";

bool ltr_int_wii_init_prefs()
{
  const char *dev = ltr_int_get_device_section();
  if(dev == NULL){
    return false;
  }
  char *tmp = ltr_int_get_key(dev, run_id_key);
  if(tmp != NULL){
    strncpy(run_indication, tmp, 5);
    free(tmp);
  }
  tmp = ltr_int_get_key(dev, pause_id_key);
  if(tmp != NULL){
    strncpy(pause_indication, tmp, 5);
    free(tmp);
  }
  return true;
}


bool ltr_int_get_run_indication(bool *d1, bool *d2, bool *d3, bool *d4)
{
  decode(run_indication, d1, d2, d3, d4);
  return true;
}

bool ltr_int_set_run_indication(bool d1, bool d2, bool d3, bool d4)
{
  encode(run_indication, d1, d2, d3, d4);
  return ltr_int_change_key(ltr_int_get_device_section(), run_id_key, run_indication);
}

bool ltr_int_get_pause_indication(bool *d1, bool *d2, bool *d3, bool *d4)
{
  decode(pause_indication, d1, d2, d3, d4);
  return true;
}

bool ltr_int_set_pause_indication(bool d1, bool d2, bool d3, bool d4)
{
  encode(pause_indication, d1, d2, d3, d4);
  return ltr_int_change_key(ltr_int_get_device_section(), pause_id_key, pause_indication);
}

