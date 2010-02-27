#ifndef LINUX_TRACK__H
#define LINUX_TRACK__H

#include <stdbool.h>
#include "pref.h"

int lt_int_init(char *cust_section);
int lt_int_shutdown(void);
int lt_int_suspend(void);
int lt_int_wakeup(void);
void lt_int_recenter(void);
int lt_int_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz);
bool lt_int_open_pref(char *key, pref_id *prf);
bool lt_int_create_pref(char *key);
float lt_int_get_flt(pref_id prf);
int lt_int_get_int(pref_id prf);
char *lt_int_get_str(pref_id prf);
bool lt_int_set_flt(pref_id *prf, float f);
bool lt_int_set_int(pref_id *prf, int i);
bool lt_int_set_str(pref_id *prf, char *str);
bool lt_int_save_prefs(void);
bool lt_int_close_pref(pref_id *prf);

void lt_int_log_message(const char *format, ...);

#endif

