#ifndef PREF_INT__H
#define PREF_INT__H

#include "utils.h"
#include "list.h"
#include "pref.h"


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

typedef struct pref_struct{
  char *section_name;
  char *key_name;
  enum {NONE, STR, FLT, INT} data_type;
  int last_read;
  union{
    char *string;
    float flt;
    int integer;
  };
} pref_struct;

bool section_exists(char *section_name);
bool key_exists(char *section_name, char *key_name);
char *get_key(char *section_name, char *key_name);

bool change_key(char *section_name, char *key_name, char *new_value);
bool dump_prefs(char *file_name);
void free_prefs();

bool set_custom_section(char *name);
bool open_pref(char *section, char *key, pref_id *prf);
float get_flt(pref_id prf);
int get_int(pref_id prf);
char *get_str(pref_id prf);
bool close_pref(pref_id *prf);

#endif
