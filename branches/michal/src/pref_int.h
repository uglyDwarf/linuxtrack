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

extern plist prefs;

void get_section_list(char **names[]);
bool section_exists(const char *section_name);
bool key_exists(const char *section_name, const char *key_name);
char *get_key(const char *section_name, const char *key_name);

bool add_section(const char *name);
bool add_key(const char *section_name, const char *key_name, const char *new_value);
bool change_key(const char *section_name, const char *key_name, const char *new_value);
bool dump_prefs(char *file_name);
void free_prefs();

bool set_custom_section(char *name);
const char *get_custom_section_name();
bool open_pref(const char *section, char *key, pref_id *prf);
bool open_pref_w_callback(const char *section, char *key, pref_id *prf, 
                          pref_callback cbk,  void *param);
float get_flt(pref_id prf);
int get_int(pref_id prf);
char *get_str(pref_id prf);

bool set_flt(pref_id *prf, float f);
bool set_int(pref_id *prf, int i);
bool set_str(pref_id *prf, char *str);

bool read_prefs(char *file, bool force_read);
bool new_prefs();
bool save_prefs();
bool close_pref(pref_id *prf);
void print_opened();

#ifdef __cplusplus
}
#endif

#endif
