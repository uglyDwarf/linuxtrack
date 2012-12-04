#ifndef TIR_DRIVER_PREFS__H
#define TIR_DRIVER_PREFS__H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

bool ltr_int_tir_init_prefs();

int ltr_int_tir_get_max_blob();
bool ltr_int_tir_set_max_blob(int val);

int ltr_int_tir_get_min_blob();
bool ltr_int_tir_set_min_blob(int val);

int ltr_int_tir_get_status_brightness();
bool ltr_int_tir_set_status_brightness(int val);

int ltr_int_tir_get_ir_brightness();
bool ltr_int_tir_set_ir_brightness(int val);

int ltr_int_tir_get_threshold();
bool ltr_int_tir_set_threshold(int val);

bool ltr_int_tir_get_status_indication();
bool ltr_int_tir_set_status_indication(bool ind);

#ifdef __cplusplus
}
#endif

#endif