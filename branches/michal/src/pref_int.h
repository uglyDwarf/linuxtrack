#ifndef PREF_INT__H
#define PREF_INT__H

#include "utils.h"
#include "list.h"
#include "pref.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
  char *key;
  char *value;
}key_val_struct;

typedef struct{
  enum {SEC_COMMENT, KEY_VAL} sec_item_type;
  union {
    char *comment;
    key_val_struct *key_val;
  };
} section_item;

typedef struct{
  char *name;
  plist contents;
} section_struct;


typedef struct{
  enum {PREF_COMMENT, SECTION} item_type;
  union {
    char *comment;
    section_struct *section;
  };
} pref_file_item;

extern plist ltr_int_prefs;

void ltr_int_get_section_list(char **names[]);
bool ltr_int_section_exists(const char *section_name);
bool ltr_int_key_exists(const char *section_name, const char *key_name);
const char *ltr_int_get_key(const char *section_name, const char *key_name);
bool ltr_int_get_key_flt(const char *section_name, const char *key_name, float *val);
bool ltr_int_get_key_int(const char *section_name, const char *key_name, int *val);

bool ltr_int_add_section(const char *name);
bool ltr_int_add_key(const char *section_name, const char *key_name, const char *new_value);
bool ltr_int_change_key(const char *section_name, const char *key_name, const char *new_value);
bool ltr_int_change_key_flt(const char *section_name, const char *key_name, float new_value);
bool ltr_int_change_key_int(const char *section_name, const char *key_name, int new_value);
bool ltr_int_dump_prefs(char *file_name);
void ltr_int_free_prefs();

bool ltr_int_set_custom_section(char *name);
const char *ltr_int_get_custom_section_name();
bool ltr_int_read_prefs(char *file, bool force_read);
bool ltr_int_new_prefs();
bool ltr_int_save_prefs();
bool ltr_int_need_saving();

#ifdef __cplusplus
}
#endif

#endif
