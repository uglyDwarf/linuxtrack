#ifndef WC_DRIVER_PREFS__H
#define WC_DRIVER_PREFS__H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

bool ltr_int_wc_init_prefs();

int ltr_int_wc_get_max_blob();
bool ltr_int_wc_set_max_blob(int val);

int ltr_int_wc_get_min_blob();
bool ltr_int_wc_set_min_blob(int val);

int ltr_int_wc_get_threshold();
bool ltr_int_wc_set_threshold(int val);

const char *ltr_int_wc_get_id();

const char *ltr_int_wc_get_pixfmt();
bool ltr_int_wc_set_pixfmt(const char *fmt);

bool ltr_int_wc_get_resolution(int *x, int *y);
bool ltr_int_wc_set_resolution(int x, int y);

bool ltr_int_wc_get_fps(int *num, int *den);
bool ltr_int_wc_set_fps(int num, int den);

bool ltr_int_wc_get_flip();
bool ltr_int_wc_set_flip(bool new_flip);

const char *ltr_int_wc_get_cascade();
bool ltr_int_wc_set_cascade(const char *new_cascade);

float ltr_int_wc_get_eff();
bool ltr_int_wc_set_eff(float new_eff);

int ltr_int_wc_get_moving_deadzone();
bool ltr_int_wc_set_moving_deadzone(int new_dz);

int ltr_int_wc_get_proc_affinity();
bool ltr_int_wc_set_proc_affinity(int new_aff);




#ifdef __cplusplus
}
#endif

#endif
