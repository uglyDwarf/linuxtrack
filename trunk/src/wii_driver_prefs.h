#ifndef WII_DRIVER_PREFS__H
#define WII_DRIVER_PREFS__H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

bool ltr_int_wii_init_prefs();
bool ltr_int_get_run_indication(bool *d1, bool *d2, bool *d3, bool *d4);
bool ltr_int_set_run_indication(bool d1, bool d2, bool d3, bool d4);
bool ltr_int_get_pause_indication(bool *d1, bool *d2, bool *d3, bool *d4);
bool ltr_int_set_pause_indication(bool d1, bool d2, bool d3, bool d4);

#ifdef __cplusplus
}
#endif

#endif