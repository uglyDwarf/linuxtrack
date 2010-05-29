/*#ifdef HAVE_CONFIG_H
  #include "../config.h"
#endif
*/
#define _GNU_SOURCE

#ifndef DARWIN
#include <features.h>
#endif

#include <stdio.h>
#undef _GNU_SOURCE
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "pref_int.h"
#include "pref.h"

int yyparse(void);
extern FILE* yyin;
extern int yydebug;
extern char* yytext;
extern section_struct *current_section;
char *parsed_file;
int line_num;

plist ltr_int_prefs;

static bool prefs_read_already = false;
static char *def_section_name = "Default";
static char *custom_section_name = NULL;
static plist opened_prefs = NULL;

typedef struct pref_data{
  char *section_name;
  char *key_name;
  //means that the value is not in cust. sec. but in def.sec
  bool defaulted; 
  bool in_def_section;
  bool invalid;
  enum {NO_TYPE, STR, FLT, INT} data_type;
  union{
    char *string;
    float flt;
    int integer;
  };
  plist refs; //list of pref_struct*
} pref_data;

typedef struct pref_struct{
  pref_data *data;
  pref_callback cbk;
  void *param;
} pref_struct;



bool ltr_int_open_pref(const char *section, char *key, pref_id *prf)
{
  return ltr_int_open_pref_w_callback(section, key, prf, NULL, NULL);
}

void ltr_int_print_opened()
{
  iterator i;
  ltr_int_init_iterator(opened_prefs, &i);
  struct pref_data* opened;
  while((opened = (struct pref_data*)ltr_int_get_next(&i)) != NULL){
    printf("Opened: %s -> %s\n", opened->section_name, opened->key_name);
  }
}

bool ltr_int_open_pref_w_callback(const char *section, char *key, pref_id *prf, 
                          pref_callback cbk,  void *param)
{
  assert(key != NULL);
  assert(prf != NULL);
  assert(section != NULL);
  
  if(opened_prefs == NULL){
    opened_prefs = ltr_int_create_list();
  }
  
  //Check, if such pref is not already opened
  iterator i;
  ltr_int_init_iterator(opened_prefs, &i);
  struct pref_data *opened;
  bool matched = false;
  while((opened = (struct pref_data*)ltr_int_get_next(&i)) != NULL){
    if((strcasecmp(section, opened->section_name) == 0) &&
       (strcasecmp(opened->key_name, key) == 0)){
      matched = true;
      break;
    }
  }
  
  if(matched){ //It is...
    *prf = (pref_struct*)ltr_int_my_malloc(sizeof(pref_struct));
    (*prf)->data = opened;
    (*prf)->cbk = cbk;
    (*prf)->param = param;
    ltr_int_add_element(opened->refs, *prf);
    return true;
  }
  
  opened = (struct pref_data*)ltr_int_my_malloc(sizeof(struct pref_data));
  opened->section_name = ltr_int_my_strdup(section);
  opened->key_name = ltr_int_my_strdup(key);
  opened->data_type = NO_TYPE;
  opened->refs = ltr_int_create_list();
  if((ltr_int_get_key(section, key) == NULL) && 
     (ltr_int_get_key(def_section_name, key) != NULL)){
    opened->defaulted = true;
  }else{
    opened->defaulted = false;
  }
  if(strcasecmp(section, def_section_name) == 0){
    opened->in_def_section = true;
  }else{
    opened->in_def_section = false;
  }
  opened->invalid = true;
  *prf = (pref_struct*)ltr_int_my_malloc(sizeof(pref_struct));
  (*prf)->cbk = cbk;
  (*prf)->param = param;
  (*prf)->data = opened;
  ltr_int_add_element(opened->refs, *prf);
  ltr_int_add_element(opened_prefs, opened);
  return true;
}


static char *get_pref_char(pref_id pref)
{
  assert(pref != NULL);
  pref_data *prf = pref->data;
  assert(prf != NULL);
  char *section = prf->section_name;
  char *key = prf->key_name;
  char *res = ltr_int_get_key(section, key);
  if(res != NULL){
    return res;
  }
  res = ltr_int_get_key(def_section_name, key);
  if(res == NULL){
    ltr_int_log_message("Trying to read pref %s in section %s\n", section, key);
  }
  return res;
}

float ltr_int_get_flt(pref_id pref)
{
  if(pref->data->data_type == NO_TYPE){
    pref->data->data_type = FLT;
  }
  assert(pref->data->data_type == FLT);
  if(pref->data->invalid){
    pref->data->invalid = false;
    char *res = get_pref_char(pref);
    if(res == NULL){
      return 3333.4444f;
    }
    pref->data->flt = atof(res);
  }
  return pref->data->flt;
}

int ltr_int_get_int(pref_id pref)
{
  if(pref->data->data_type == NO_TYPE){
    pref->data->data_type = INT;
  }
  assert(pref->data->data_type == INT);
  if(pref->data->invalid){
    pref->data->invalid = false;
    char *res = get_pref_char(pref);
    if(res == NULL){
      return 5555;
    }
    pref->data->integer = atoi(res);
  }
  return pref->data->integer;
}

char *ltr_int_get_str(pref_id pref)
{
  if(pref->data->data_type == NO_TYPE){
    pref->data->data_type = STR;
  }
  assert(pref->data->data_type == STR);
  if(pref->data->invalid){
    pref->data->invalid = false;
    char *res = get_pref_char(pref);
    if(res == NULL){
      return ltr_int_my_strdup("DEADBEEF");
    }
    pref->data->string = ltr_int_my_strdup(res);
  }
  return pref->data->string;
}

static void notify_refs(plist refs)
{
  iterator i;
  ltr_int_init_iterator(refs, &i);
  pref_struct* ref;
  while((ref = (pref_struct*)ltr_int_get_next(&i)) != NULL){
    if(ref->cbk != NULL){
      (ref->cbk)(ref->param);
    }
  }
}

static void mark_pref_changed(pref_id *prf)
{
  assert(prf != NULL);
  notify_refs((*prf)->data->refs);
  if((*prf)->data->in_def_section){
    iterator j;
    ltr_int_init_iterator(opened_prefs, &j);
    struct pref_data *opened;
    while((opened = (struct pref_data*)ltr_int_get_next(&j)) != NULL){
      if((opened->defaulted) && 
         (strcasecmp(opened->key_name, (*prf)->data->key_name) == 0)){
	opened->invalid = true;
        notify_refs(opened->refs);
      }
    }
  }
}


bool ltr_int_set_flt(pref_id *pref, float f)
{
  assert(pref != NULL);
  pref_data *prf = (*pref)->data;
  assert(prf != NULL);
  char *section = prf->section_name;
  char *key = prf->key_name;
  if(prf->data_type == NO_TYPE){
    prf->data_type = FLT;
  }
  assert(prf->data_type == FLT);
  char *new_val;
  asprintf(&new_val, "%f", f);
  bool rv = ltr_int_change_key(section, key, new_val);
  free(new_val);
  if(rv == true){
    prf->flt = f;
    mark_pref_changed(pref);
  }
  return rv;
}

bool ltr_int_set_int(pref_id *pref, int i)
{
  assert(pref != NULL);
  pref_data *prf = (*pref)->data;
  assert(prf != NULL);
  char *section = prf->section_name;
  char *key = prf->key_name;
  if(prf->data_type == NO_TYPE){
    prf->data_type = INT;
  }
  assert(prf->data_type == INT);
  char *new_val;
  asprintf(&new_val, "%d", i);
  bool rv = ltr_int_change_key(section, key, new_val);
  free(new_val);
  if(rv == true){
    prf->integer = i;
    mark_pref_changed(pref);
  }
  return rv;
}

bool ltr_int_set_str(pref_id *pref, char *str)
{
  bool has_payload = true;
  assert(pref != NULL);
  pref_data *prf = (*pref)->data;
  assert(prf != NULL);
  char *section = prf->section_name;
  char *key = prf->key_name;
  if(prf->data_type == NO_TYPE){
    prf->data_type = STR;
    has_payload = false;
  }
  assert(prf->data_type == STR);
  bool rv = ltr_int_change_key(section, key, str);
  if(rv == true){
    if(has_payload){
      free(prf->string);
    }
    prf->string = ltr_int_my_strdup(str);
    mark_pref_changed(pref);
  }
  return rv;
}


bool ltr_int_close_pref(pref_id *pref)
{
  assert(pref != NULL);
  pref_data *prf = (*pref)->data;
  assert(prf != NULL);
  //print_opened();
  //Find pref in the list of opened prefs
  iterator i;
  ltr_int_init_iterator(opened_prefs, &i);
  struct pref_data* opened;
  bool matched = false;
  while((opened = (struct pref_data*)ltr_int_get_next(&i)) != NULL){
    if(opened == prf){
        matched = true;
        break;
    }
  }
  
  assert(matched);
  
  iterator j;
  ltr_int_init_iterator(opened->refs, &j);
  pref_struct *p;
  matched = false;
  while((p = (pref_struct*)ltr_int_get_next(&j)) != NULL){
    if(p == *pref){
      matched = true;
      break;
    }
  }
  
  assert(matched);
  if(opened->refs)
  ltr_int_delete_current(opened->refs, &j);
  
  if(ltr_int_is_empty(opened->refs)){
    ltr_int_free_list(opened->refs, false);
    if(prf->section_name != NULL){
      free(prf->section_name);
      prf->section_name = NULL;
    }
    free(prf->key_name);
    prf->key_name = NULL;
    if(prf->data_type == STR){
      free(prf->string);
    }
    opened = (struct pref_data*)ltr_int_delete_current(opened_prefs, &i);
    assert(opened != NULL);
    free(opened);
  }
  if(ltr_int_is_empty(opened_prefs)){
    ltr_int_free_list(opened_prefs, false);
    opened_prefs = NULL;
  }
  free(*pref);
  *pref = NULL;
  return true;
}



static bool parse_prefs(char *fname)
{
  ltr_int_prefs = ltr_int_create_list();
  if((yyin=fopen(fname, "r")) != NULL){
    parsed_file = ltr_int_my_strdup(fname);
    line_num = 1;
    current_section = NULL;
    //yydebug=1;
    int res = yyparse();
    fclose(yyin);
    free(parsed_file);
    parsed_file = NULL;
    if(res == 0){
      ltr_int_log_message("Preferences read OK!\n");
      return(true);
    }
  }
  ltr_int_log_message("Error encountered while reading preferences!\n");
  ltr_int_free_prefs();
  return(false);
}

void yyerror(char const *s)
{
  ltr_int_log_message("%s in file %s, line %d near '%s'\n",
  		 s, parsed_file, line_num, yytext);
}

bool ltr_int_read_prefs(char *file, bool force_read)
{
  static bool prefs_ok = false;
  bool free_str = false;
  
  if((prefs_read_already == false) || (force_read == true)){
    char *pfile;
    if(file != NULL){
      pfile = file;
    }else{
      pfile = ltr_int_get_default_file_name();
      free_str = true;
    }
    if(pfile == NULL){
      return false;
    }
    prefs_ok = parse_prefs(pfile);
    if(free_str){
      free(pfile);
    }
    if(prefs_ok){
      prefs_read_already = true;
    }
  }
  return prefs_ok;
}

bool ltr_int_new_prefs()
{
  if(prefs_read_already){
    ltr_int_free_prefs();
  }
  ltr_int_prefs = ltr_int_create_list();
  prefs_read_already = true;
  return true;
}

section_struct *ltr_int_find_section(const char *section_name)
{
  assert(prefs_read_already);
  assert(section_name != NULL);
  
  iterator i;
  ltr_int_init_iterator(ltr_int_prefs, &i);
  
  pref_file_item *pfi;
  while((pfi = (pref_file_item *)ltr_int_get_next(&i)) != NULL){
    if(pfi->item_type == SECTION){
      assert(pfi->section != NULL);
      if(strcasecmp(pfi->section->name, section_name) == 0){
        return pfi->section;
      }
    }
  }
  return NULL;
}


void ltr_int_get_section_list(char **names[])
{
  assert(prefs_read_already);
  assert(names != NULL);
  
  plist sections = ltr_int_create_list();
  iterator i;
  ltr_int_init_iterator(ltr_int_prefs, &i);
  
  pref_file_item *pfi;
  while((pfi = (pref_file_item *)ltr_int_get_next(&i)) != NULL){
    if(pfi->item_type == SECTION){
      assert(pfi->section != NULL);
      ltr_int_add_element(sections, ltr_int_my_strdup(pfi->section->name));
    }
  }
  ltr_int_list2string_list(sections, names);
  return;
}

key_val_struct *ltr_int_find_key(const char *section_name, const char *key_name)
{
  assert(prefs_read_already);
  assert(section_name != NULL);
  assert(key_name != NULL);
  
  section_struct *sec = ltr_int_find_section(section_name);
  if(sec == NULL){
    ltr_int_log_message("Section %s not found!\n", section_name);
    return NULL;
  }
  iterator i;
  ltr_int_init_iterator(sec->contents, &i);
  
  section_item *si;
  while((si = (section_item *)ltr_int_get_next(&i)) != NULL){
    if(si->sec_item_type == KEY_VAL){
      assert(si->key_val != NULL);
      if(strcasecmp(si->key_val->key, key_name) == 0){
        return si->key_val;
      }
    }
  }
  return NULL;
}

bool ltr_int_section_exists(const char *section_name)
{
  assert(prefs_read_already);
  if(ltr_int_find_section(section_name) != NULL){
    return true;
  }else{
    return false;
  }
}

bool ltr_int_key_exists(const char *section_name, const char *key_name)
{
  assert(prefs_read_already);
  if(ltr_int_find_key(section_name, key_name) != NULL){
    return true;
  }else{
    return false;
  }
}

char *ltr_int_get_key(const char *section_name, const char *key_name)
{
  assert(prefs_read_already);
  key_val_struct *kv = NULL;
  if(section_name != NULL){
    kv = ltr_int_find_key(section_name, key_name);
  }else{
    if((custom_section_name != NULL) && 
       (ltr_int_key_exists(custom_section_name, key_name) == true)){
      kv = ltr_int_find_key(custom_section_name, key_name);
    }else if(ltr_int_key_exists(def_section_name, key_name) == true){
      kv = ltr_int_find_key(def_section_name, key_name);
    }else{
      ltr_int_log_message("Attempted to get nonexistent key '%s'\n", key_name);
    }
  }
  
  if(kv == NULL){
    return NULL;
  }
  return kv->value;
}


bool ltr_int_add_key(const char *section_name, const char *key_name, const char *new_value)
{
  assert(prefs_read_already);
  if(section_name == NULL){
    section_name = custom_section_name;
  }
  section_struct *section = ltr_int_find_section(section_name);
  if(section == NULL){
    ltr_int_log_message("Attempted to add key to nonexistent section %s!\n", 
      (section_name != NULL? section_name : "'NULL'"));
    return false;
  }
  key_val_struct *kv = (key_val_struct*)ltr_int_my_malloc(sizeof(key_val_struct));
  kv->key = ltr_int_my_strdup(key_name);
  kv->value = ltr_int_my_strdup(new_value);
  
  section_item *item = (section_item*)ltr_int_my_malloc(sizeof(section_item));
  item->sec_item_type = KEY_VAL;
  item->key_val = kv;
  
  ltr_int_add_element(section->contents, item);
  return true;
}

bool ltr_int_change_key(const char *section_name, const char *key_name, const char *new_value)
{
  assert(prefs_read_already);
  assert(key_name != NULL);
  assert(new_value != NULL);
  
  if(section_name == NULL){
    assert(custom_section_name != NULL);
    section_name = custom_section_name;
  }
  key_val_struct *kv = NULL;
  kv = ltr_int_find_key(section_name, key_name);
  if(kv == NULL){
    return ltr_int_add_key(section_name, key_name, new_value);
  }else{
    free(kv->value);
    kv->value = ltr_int_my_strdup(new_value);
  }
  return true;
}



bool ltr_int_dump_section(section_struct *section, FILE *of)
{
  fprintf(of, "[%s]\n", section->name);
  
  iterator i;
  ltr_int_init_iterator(section->contents, &i);
  
  section_item *sci;
  while((sci = (section_item *)ltr_int_get_next(&i)) != NULL){
    switch(sci->sec_item_type){
      case KEY_VAL:
        fprintf(of,"%s = %s\n", sci->key_val->key, sci->key_val->value);
        break;
      case SEC_COMMENT:
        fprintf(of,"%s\n", sci->comment);
        break;
      default:
        assert(0);
        break;
    }
  }
  return true;
}

bool ltr_int_dump_prefs(char *file_name)
{
  FILE *of;
  if(file_name == NULL){
    of = stdout;
  }else{
    of = fopen(file_name, "w");
    if(of == NULL){
      ltr_int_log_message("Can't open file '%s'!\n", file_name);
      return false;
    }
  }
  assert(prefs_read_already);
  iterator i;
  ltr_int_init_iterator(ltr_int_prefs, &i);
  
  pref_file_item *pfi;
  while((pfi = (pref_file_item *)ltr_int_get_next(&i)) != NULL){
    switch(pfi->item_type){
      case SECTION:
        fprintf(of, "\n");
        ltr_int_dump_section(pfi->section, of);
        break;
      case PREF_COMMENT:
        fprintf(of,"%s\n", pfi->comment);
        break;
    }
  }
  if(file_name != NULL){
    fclose(of);
  }
  return true;
}

static void free_section(section_struct *section)
{
  free(section->name);
  
  iterator i;
  ltr_int_init_iterator(section->contents, &i);
  
  section_item *sci;
  while((sci = (section_item *)ltr_int_get_next(&i)) != NULL){
    switch(sci->sec_item_type){
      case KEY_VAL:
        free(sci->key_val->key);
        free(sci->key_val->value);
        free(sci->key_val);
        break;
      case SEC_COMMENT:
        free(sci->comment);
        break;
      default:
	assert(0);
	break;
    }
    free(sci);
  }
  ltr_int_free_list(section->contents, false);
}

void ltr_int_free_prefs()
{
  iterator i;
  assert(ltr_int_prefs != NULL);
  ltr_int_init_iterator(ltr_int_prefs, &i);
  
  pref_file_item *pfi;
  while((pfi = (pref_file_item *)ltr_int_get_next(&i)) != NULL){
    switch(pfi->item_type){
      case SECTION:
        free_section(pfi->section);
        free(pfi->section);
        break;
      case PREF_COMMENT:
        free(pfi->comment);
        break;
      default:
	assert(0);
	break;
    }
    free(pfi);
  }
  ltr_int_free_list(ltr_int_prefs, false);
  prefs_read_already = false;
}

bool ltr_int_set_custom_section(char *name)
{
  if(name != NULL){
    assert(prefs_read_already);
    assert(name != NULL);
    //Find section with given title...
    iterator i;
    ltr_int_init_iterator(ltr_int_prefs, &i);
    char *title, *sec_name;
    bool found = false;
    pref_file_item *pfi;
    while((pfi = (pref_file_item *)ltr_int_get_next(&i)) != NULL){
      if(pfi->item_type == SECTION){
        assert(pfi->section != NULL);
        sec_name = pfi->section->name;
        title = ltr_int_get_key(sec_name, "Title");
        if(title != NULL){
          if(strcasecmp(title, name) == 0){
            found = true;
            break;
          }
        }
      }
    }
    
    if(found){
      custom_section_name = ltr_int_my_strdup(sec_name);
      ltr_int_log_message("Custom section '%s' found!\n", sec_name);
    }else{
      ltr_int_log_message("Attempted to set nonexistent section '%s'!\n", name);
      return false;
    }
  }else{
    if(custom_section_name != NULL){
      free(custom_section_name);
    }
    custom_section_name = NULL;
  }
  return true;
}

const char *ltr_int_get_custom_section_name()
{
  return custom_section_name;
}

bool ltr_int_save_prefs()
{
  char *pfile = ltr_int_get_default_file_name();
  if(pfile == NULL){
    ltr_int_log_message("Can't find preference file!\n");
    return false;
  }
  char *tmp_file = NULL;
  char *old_file = NULL;
  asprintf(&tmp_file, "%s.new", pfile);
  if(ltr_int_dump_prefs(tmp_file) == true){
    asprintf(&old_file, "%s.old", pfile);
    remove(old_file);
    if(rename(pfile, old_file) != 0){
      ltr_int_log_message("Can't rename '%s' to '%s'\n", old_file, pfile);
    }
    if(rename(tmp_file, pfile) != 0){
      ltr_int_log_message("Can't rename '%s' to '%s'\n", tmp_file, pfile);
      free(tmp_file);
      free(old_file);
      free(pfile);
      return false;
    }
  }else{
    ltr_int_log_message("Can't write prefs to file '%s'\n", tmp_file);
    free(tmp_file);
      free(pfile);
    return false;
  }
  if(tmp_file != NULL) free(tmp_file);
  if(old_file != NULL) free(old_file);
  free(pfile);
  return true;
}

bool ltr_int_add_section(const char *name)
{
  section_struct *new_section = 
    (section_struct*)ltr_int_my_malloc(sizeof(section_struct));
  new_section->name = ltr_int_my_strdup(name);
  new_section->contents = ltr_int_create_list();
  pref_file_item *item = 
    (pref_file_item*)ltr_int_my_malloc(sizeof(pref_file_item));
  item->item_type = SECTION;
  item->section = new_section;
  ltr_int_add_element(ltr_int_prefs, item);
  return true;
}
