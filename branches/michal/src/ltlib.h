#ifndef LINUX_TRACK__H
#define LINUX_TRACK__H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pref_struct *pref_id;
typedef void (*pref_callback)(void *);


int lt_init(char *cust_section);
int lt_shutdown(void);
int lt_suspend(void);
int lt_wakeup(void);
void lt_recenter(void);
int lt_get_camera_update(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz);
bool lt_open_pref(char *key, pref_id *prf);
bool lt_create_pref(char *key);
float lt_get_flt(pref_id prf);
int lt_get_int(pref_id prf);
char *lt_get_str(pref_id prf);
bool lt_set_flt(pref_id *prf, float f);
bool lt_set_int(pref_id *prf, int i);
bool lt_set_str(pref_id *prf, char *str);
bool lt_save_prefs(void);
bool lt_close_pref(pref_id *prf);

void lt_log_message(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif

