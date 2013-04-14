#ifndef PREF__H
#define PREF__H

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

char *ltr_int_get_key(const char *section_name, const char *key_name);
bool ltr_int_get_key_flt(const char *section_name, const char *key_name, float *val);
bool ltr_int_get_key_int(const char *section_name, const char *key_name, int *val);

bool ltr_int_change_key(const char *section_name, const char *key_name, const char *new_value);
bool ltr_int_change_key_flt(const char *section_name, const char *key_name, float new_value);
bool ltr_int_change_key_int(const char *section_name, const char *key_name, int new_value);
void ltr_int_free_prefs(void);

bool ltr_int_read_prefs(const char *file, bool force_read);
bool ltr_int_new_prefs(void);
bool ltr_int_save_prefs(const char *fname);
bool ltr_int_dump_prefs(const char *file_name);

bool ltr_int_need_saving(void);

char *ltr_int_find_section(const char *key_name, const char *value);
//Stupid trick - result is pointer to std::vector<std::string>
bool ltr_int_find_sections(const char *key_name, void *result);
char *ltr_int_add_unique_section(const char *name_template);

bool ltr_int_read_prefs(const char *file, bool force_read);
void ltr_int_prefs_changed(void);
//Stupid trick - sections is pointer to std::vector<std::string>
void ltr_int_get_section_list(void *sections_ptr);


#ifdef __cplusplus
}
#endif


#endif

