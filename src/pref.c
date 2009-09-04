#define _GNU_SOURCE
#include <features.h>
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

char *parsed_file;
int line_num;

plist prefs;

bool read_already = false;
char *pref_file = ".linuxtrack";
char *def_section_name = "Default";
char *custom_section_name = NULL;
int read_counter = 0;
plist opened_prefs = NULL;

struct opened{
  char *section;
  char *key;
  pref_data prf;
};



bool name_match(char *n1, char *n2){
  if(n1 == NULL){
    if(n2 == NULL){
      return true;
    }else{
      return false;
    }
  }else{
    if(n2 == NULL){
      return false;
    }else{
      if(strcmp(n1,n2) == 0){
        return true;
      }else{
        return false;
      }
    }
  }
}

bool open_pref(char *section, char *key, pref_id *prf)
{

  printf("Request to open %s\n", key);
  if(opened_prefs == NULL){
    opened_prefs = create_list();
  }
  
  iterator i;
  init_iterator(opened_prefs, &i);
  
  struct opened* o;
  bool matched = false;
  while((o = (struct opened*)get_next(&i)) != NULL){
    if(name_match(section, o->section)){
      if(name_match(key, o->key)){
        matched = true;
        break;
      }
    }
  }

  if(matched){
    *prf = (pref_struct*)my_malloc(sizeof(pref_struct));
    (*prf)->data = &(o->prf);
    (*prf)->changed = true;
    add_element(o->prf.refs, *prf);
    return true;
  }
  
  printf("Creating new...\n");
  if(get_key(section, key) == NULL){
    return false;
  }
  *prf = (pref_struct*)my_malloc(sizeof(pref_struct));
  (*prf)->changed = true;

  o = (struct opened*)my_malloc(sizeof(struct opened));
  o->section = (section != NULL) ? my_strdup(section) : NULL;
  o->key = my_strdup(key);
  if(section != NULL){// Check it !!!
    o->prf.section_name = my_strdup(section);
  }else{
    o->prf.section_name = NULL;
  }
  o->prf.key_name = my_strdup(key);
  o->prf.data_type = NONE;
  o->prf.refs = create_list();
  (*prf)->data = &(o->prf);
  add_element(o->prf.refs, *prf);
  add_element(opened_prefs, o);
  
  return true;
}

bool pref_changed(pref_id pref)
{
  return pref->changed;
}

float get_flt(pref_id pref)
{
  pref_data *prf = pref->data;
  if(prf == NULL){
    log_message("Null float preference queried!\n");
    return 0.0f;
  }
  char *section = prf->section_name;
  char *key = prf->key_name;
  if(prf->data_type == NONE){
    prf->data_type = FLT;
    pref->changed = false;
    prf->flt = atof(get_key(section, key));
  }else{
    if(prf->data_type != FLT){
      log_message("Preference %s is not float (%d)!\n", key, prf->data_type);
      return 0.0f;
    }
    if(pref->changed == true){
      prf->flt = atof(get_key(section, key));
      pref->changed = false;
    }
  }
  return prf->flt;
}

int get_int(pref_id pref)
{
  pref_data *prf = pref->data;
  if(prf == NULL){
    log_message("Null int preference queried!\n");
    return 0;
  }
  char *section = prf->section_name;
  char *key = prf->key_name;
  if(prf->data_type == NONE){
    prf->data_type = INT;
    pref->changed = false;
    prf->integer = atoi(get_key(section, key));
  }else{
    if(prf->data_type != INT){
      log_message("Preference %s is not int!\n", key);
      return 0;
    }
    if(pref->changed == true){
      prf->integer = atoi(get_key(section, key));
      pref->changed = false;
    }
  }
  return prf->integer;
}

char *get_str(pref_id pref)
{
  pref_data *prf = pref->data;
  if(prf == NULL){
    log_message("Null str preference queried!\n");
    return NULL;
  }
  char *section = prf->section_name;
  char *key = prf->key_name;
  if(prf->data_type == NONE){
    prf->data_type = STR;
    pref->changed = false;
    prf->string = get_key(section, key);
  }else{
    if(prf->data_type != STR){
      log_message("Preference %s is not string!\n", key);
      return 0;
    }
    if(pref->changed == true){
      prf->string = get_key(section, key);
      pref->changed = false;
    }
  }
  return prf->string;
}

void mark_pref_changed(pref_id *prf)
{
  if(prf == NULL){
    log_message("Tried to mark NULL preference as changed!\n");
    return;
  }
  iterator i;
  init_iterator((*prf)->data->refs, &i);
  
  pref_struct* ref;
  while((ref = (pref_struct*)get_next(&i)) != NULL){
    ref->changed = true;
  }
}

bool set_flt(pref_id *pref, float f)
{
  pref_data *prf = (*pref)->data;
  if(prf == NULL){
    log_message("Tried to set null float preference!\n");
    return false;
  }
  char *section = prf->section_name;
  char *key = prf->key_name;
  if(prf->data_type == NONE){
    prf->data_type = FLT;
    mark_pref_changed(pref);
  }else{
    if(prf->data_type != FLT){
      log_message("Preference %s is not float!\n", key);
      return false;
    }
  }
  char *new_val;
  asprintf(&new_val, "%f", f);
  bool rv = change_key(section, key, new_val);
  free(new_val);
  if(rv == true){
    prf->flt = f;
  }
  return rv;
}

bool set_int(pref_id *pref, int i)
{
  pref_data *prf = (*pref)->data;
  if(prf == NULL){
    log_message("Tried to set null int preference!\n");
    return false;
  }
  char *section = prf->section_name;
  char *key = prf->key_name;
  if(prf->data_type == NONE){
    prf->data_type = INT;
    mark_pref_changed(pref);
  }else{
    if(prf->data_type != INT){
      log_message("Preference %s is not int!\n", key);
      return false;
    }
  }
  char *new_val;
  asprintf(&new_val, "%d", i);
  bool rv = change_key(section, key, new_val);
  free(new_val);
  if(rv == true){
    prf->integer = i;
  }
  return rv;
}

bool set_str(pref_id *pref, char *str)
{
  pref_data *prf = (*pref)->data;
  if(prf == NULL){
    log_message("Tried to set null str preference!\n");
    return false;
  }
  char *section = prf->section_name;
  char *key = prf->key_name;
  if(prf->data_type == NONE){
    prf->data_type = STR;
    mark_pref_changed(pref);
  }else{
    if(prf->data_type != STR){
      log_message("Preference %s is not string!\n", key);
      return false;
    }
  }
  bool rv = change_key(section, key, prf->string);
  if(rv == true){
    prf->string = my_strdup(str);
  }
  return rv;
}


bool close_pref(pref_id *pref)
{
  printf("Closing pref!\n");
  if(pref == NULL){
    log_message("Trying to close null pref.\n");
  }
  pref_data *prf = (*pref)->data;
  
  iterator i;
  init_iterator(opened_prefs, &i);
  
  struct opened* o;
  bool matched = false;
  while((o = (struct opened*)get_next(&i)) != NULL){
    if(&(o->prf) == prf){
        matched = true;
        break;
    }
  }
  if(matched == false){
    log_message("Trying to close already closed preference!\n");
    return false;
  }
  delete_current(o->prf.refs, &i);
  if(is_empty(o->prf.refs)){
    free_list(o->prf.refs, false);
    if(prf->section_name != NULL){
      free(prf->section_name);
      prf->section_name = NULL;
    }
    free(prf->key_name);
    prf->key_name = NULL;
    o = (struct opened*)delete_current(opened_prefs, &i);
    assert(o != NULL);
    if(o->section != NULL){
      free(o->section);
    }
    free(o->key);
  }
  *pref = NULL;
  return true;
}



bool read_prefs(char *fname, char *section)
{
  prefs = create_list();
  if((yyin=fopen(fname, "r")) != NULL){
    parsed_file = my_strdup(fname);
    line_num = 1;
    //yydebug=1;
    int res = yyparse();
    fclose(yyin);
    free(parsed_file);
    parsed_file = NULL;
    if(res == 0){
      log_message("Preferences read OK!\n");
      ++read_counter;
      return(true);
    }
  }
  log_message("Error encountered while reading preferences!\n");
  return(false);
}

void yyerror(char const *s)
{
  log_message("%s in file %s, line %d near '%s'\n",
  		 s, parsed_file, line_num, yytext);
}

char *get_pref_file_name()
{
  char *home = getenv("HOME");
  if(home == NULL){
    log_message("Please set HOME variable!'n");
    return NULL;
  }
  char *pref_path = (char *)my_malloc(strlen(home) 
                    + strlen(pref_file) + 2);
  sprintf(pref_path, "%s/%s", home, pref_file);
  return pref_path;
}

bool read_prefs_on_init()
{
  static bool read_already = false;
  static bool prefs_ok = false;
  if(read_already == false){
    char *pfile = get_pref_file_name();
    if(pfile == NULL){
      return false;
    }
    prefs_ok = read_prefs(pfile, "");
    free(pfile);
    read_already = true;
  }
  return prefs_ok;
}

section_struct *find_section(char *section_name)
{
  if(read_prefs_on_init() == false){
    return NULL;
  }
  if(section_name == NULL){
    log_message("Attempt to find section with NULL name!");
    return false;
  }
  iterator i;
  init_iterator(prefs, &i);
  
  pref_file_item *pfi;
  while((pfi = (pref_file_item *)get_next(&i)) != NULL){
    if(pfi->item_type == SECTION){
      assert(pfi->section != NULL);
      if(strcasecmp(pfi->section->name, section_name) == 0){
        return pfi->section;
      }
    }
  }
  return NULL;
}

key_val_struct *find_key(char *section_name, char *key_name)
{
  if(read_prefs_on_init() == false){
    return NULL;
  }
  section_struct *sec = find_section(section_name);
  if(sec == NULL){
    log_message("Section %s not found!\n", section_name);
    return NULL;
  }
  iterator i;
  init_iterator(sec->contents, &i);
  
  section_item *si;
  while((si = (section_item *)get_next(&i)) != NULL){
    if(si->sec_item_type == KEY_VAL){
      assert(si->key_val != NULL);
      if(strcasecmp(si->key_val->key, key_name) == 0){
        return si->key_val;
      }
    }
  }
  return NULL;
}

bool section_exists(char *section_name)
{
  if(read_prefs_on_init() == false){
    return false;
  }
  if(find_section(section_name) != NULL){
    return true;
  }else{
    return false;
  }
}

bool key_exists(char *section_name, char *key_name)
{
  if(read_prefs_on_init() == false){
    return false;
  }
  if(find_key(section_name, key_name) != NULL){
    return true;
  }else{
    return false;
  }
}

char *get_key(char *section_name, char *key_name)
{
  if(read_prefs_on_init() == false){
    return NULL;
  }
  printf("Getkey %s %s\n", section_name!=NULL?section_name:"NULL", key_name);
  key_val_struct *kv = NULL;
  if(section_name != NULL){
    kv = find_key(section_name, key_name);
  }else{
    if((custom_section_name != NULL) && 
       (key_exists(custom_section_name, key_name) == true)){
      kv = find_key(custom_section_name, key_name);
    }else if(key_exists(def_section_name, key_name) == true){
      kv = find_key(def_section_name, key_name);
    }else{
      log_message("Attempted to get nonexistent key '%s'\n", key_name);
    }
  }
  
  if(kv == NULL){
    return NULL;
  }
  return kv->value;
}


bool add_key(char *section_name, char *key_name, char *new_value)
{
  if(read_prefs_on_init() == false){
    return NULL;
  }
  if(section_name == NULL){
    section_name = custom_section_name;
  }
  section_struct *section = find_section(section_name);
  if(section == NULL){
    log_message("Attempted to add key to nonexistent section %s!\n", 
      (section_name != NULL? section_name : "'NULL'"));
    return false;
  }
  key_val_struct *kv = (key_val_struct*)my_malloc(sizeof(key_val_struct));
  kv->key = my_strdup(key_name);
  kv->value = my_strdup(new_value);
  
  section_item *item = (section_item*)my_malloc(sizeof(section_item));
  item->sec_item_type = KEY_VAL;
  item->key_val = kv;
  
  add_element(section->contents, item);
  return true;
}

bool change_key(char *section_name, char *key_name, char *new_value)
{
  if(read_prefs_on_init() == false){
    return NULL;
  }
  key_val_struct *kv = NULL;
  if(section_name != NULL){
    if(section_exists(section_name)){
      kv = find_key(section_name, key_name);
      if(kv == NULL){
        return add_key(section_name, key_name, new_value);
      }
    }else{
      log_message("Section %s doesn't exist!\n", section_name);
      return false;
    }
  }else{
    if(custom_section_name != NULL){
      if(key_exists(custom_section_name, key_name) == true){
        kv = find_key(custom_section_name, key_name);
      }else{
        return add_key(custom_section_name, key_name, new_value);
      }
    }else{
      log_message("Set custom section name before change of pref %s!\n", key_name);
      return false;
    }
  }
  if(kv == NULL){
    return false;
  }
  free(kv->value);
  kv->value = my_strdup(new_value);
  return true;
}



bool dump_section(section_struct *section, FILE *of)
{
  fprintf(of, "[%s]\n", section->name);
  
  iterator i;
  init_iterator(section->contents, &i);
  
  section_item *sci;
  while((sci = (section_item *)get_next(&i)) != NULL){
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

bool dump_prefs(char *file_name)
{
  FILE *of;
  if(file_name == NULL){
    of = stderr;
  }else{
    of = fopen(file_name, "w");
    if(of == NULL){
      log_message("Can't open file '%s'!\n", file_name);
      return false;
    }
  }
  if(read_prefs_on_init() == false){
    return false;
  }
  iterator i;
  init_iterator(prefs, &i);
  
  pref_file_item *pfi;
  while((pfi = (pref_file_item *)get_next(&i)) != NULL){
    switch(pfi->item_type){
      case SECTION:
        fprintf(of, "\n");
        dump_section(pfi->section, of);
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

void free_section(section_struct *section)
{
  free(section->name);
  
  iterator i;
  init_iterator(section->contents, &i);
  
  section_item *sci;
  while((sci = (section_item *)get_next(&i)) != NULL){
    switch(sci->sec_item_type){
      case KEY_VAL:
        free(sci->key_val->key);
        free(sci->key_val->value);
        free(sci->key_val);
        break;
      case SEC_COMMENT:
        free(sci->comment);
        break;
    }
    free(sci);
  }
  free_list(section->contents, false);
}

void free_prefs()
{
  iterator i;
  init_iterator(prefs, &i);
  
  pref_file_item *pfi;
  while((pfi = (pref_file_item *)get_next(&i)) != NULL){
    switch(pfi->item_type){
      case SECTION:
        free_section(pfi->section);
        free(pfi->section);
        break;
      case PREF_COMMENT:
        free(pfi->comment);
        break;
    }
    free(pfi);
  }
  free_list(prefs, false);
}

bool set_custom_section(char *name)
{
  if(name != NULL){
    if(section_exists(name) == true){
      custom_section_name = my_strdup(name);
      log_message("Custom section '%s' found!\n", name);
      return true;
    }else{
      log_message("Attempted to set nonexistent section '%s'!\n", name);
    }
  }
  return false;
}

bool save_prefs()
{
  char *pfile = get_pref_file_name();
  if(pfile == NULL){
    log_message("Can't find preference file!\n");
  }
  char *tmp_file;
  asprintf(&tmp_file, "%s.new", pfile);
  if(dump_prefs(tmp_file) == true){
    char *old_file;
    asprintf(&old_file, "%s.old", pfile);
    remove(old_file);
    if(rename(pfile, old_file) == 0){
      if(rename(tmp_file, pfile) != 0){
        log_message("Can't rename '%s' to '%s'\n", tmp_file, pfile);
        return false;
      }
    }else{
    }
  }else{
    log_message("Can't write prefs to file '%s'\n", tmp_file);
    return false;
  }
  return true;
}

/*
int main(int argc, char *argv[])
{
  set_custom_section("XPlane");
  printf("Device type: %s\n", get_key("Global", "Capture-device"));
  printf("Head ref [0, %s, %s]\n", get_key("Global", "Head-Y"), 
  	get_key("Global", "Head-Z"));
  dump_prefs("pref1.dmp");
  change_key("Global", "Head-Y", "333");
  change_key("Global", "Head-Z", "444");
  printf("Head ref [0, %s, %s]\n", get_key("Global", "Head-Y"), 
  	get_key("Global", "Head-Z"));
  
  pref_id ff, fb, fc;
  if(open_pref(NULL, "Filter-factor", &ff)){
    printf("Pref OK... %f\n", get_flt(ff));
    if(set_flt(&ff, 3.14))
      printf("Pref OK... %f\n", get_flt(ff));
    close_pref(&ff);
  }
  if(open_pref(NULL, "Recenter-button", &fb)){
    printf("Pref OK... %d\n", get_int(fb));
    if(set_int(&fb, 14))
      printf("Pref OK... %d\n", get_int(fb));
    close_pref(&fb);
  }
  dump_prefs("pref2.dmp");
  save_prefs();
  
  free_prefs();
  return 0;
}
//gcc -o pt -g pref.c utils.c list.c pref_bison.c pref_flex.c; ./pt


*/

