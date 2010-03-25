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

typedef struct pref_data{
  char *section_name;
  char *key_name;
  enum {NONE, STR, FLT, INT} data_type;
  union{
    char *string;
    float flt;
    int integer;
  };
  plist refs;
} pref_data;

typedef struct pref_struct{
  pref_data *data;
  pref_callback cbk;
  void *param;
} pref_struct;

void get_section_list(char **names[]);
bool section_exists(char *section_name);
bool key_exists(char *section_name, char *key_name);
char *get_key(char *section_name, char *key_name);

bool add_section(char *name);
bool add_key(char *section_name, char *key_name, char *new_value);
bool change_key(char *section_name, char *key_name, char *new_value);
bool dump_prefs(char *file_name);
void free_prefs();

bool set_custom_section(char *name);
bool open_pref(char *section, char *key, pref_id *prf);
bool open_pref_w_callback(char *section, char *key, pref_id *prf, 
                          pref_callback cbk,  void *param);
float get_flt(pref_id prf);
int get_int(pref_id prf);
char *get_str(pref_id prf);

bool set_flt(pref_id *prf, float f);
bool set_int(pref_id *prf, int i);
bool set_str(pref_id *prf, char *str);

bool read_prefs(char *file, bool force_read);
bool save_prefs();
bool close_pref(pref_id *prf);

#ifdef __cplusplus
}
#endif

#endif
