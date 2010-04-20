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

plist prefs;

bool prefs_read_already = false;
char *pref_file = ".linuxtrack";
char *def_section_name = "Default";
char *custom_section_name = NULL;
static plist opened_prefs = NULL;

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
  return open_pref_w_callback(section, key, prf, NULL, NULL);
}

void print_opened()
{
  iterator i;
  init_iterator(opened_prefs, &i);
  struct opened* o;
  while((o = (struct opened*)get_next(&i)) != NULL){
    printf("Opened: %s -> %s\n", o->section, o->key);
  }
}

bool open_pref_w_callback(char *section, char *key, pref_id *prf, 
                          pref_callback cbk,  void *param)
{
  assert(key != NULL);
  assert(prf != NULL);
  if(opened_prefs == NULL){
    opened_prefs = create_list();
  }
  //print_opened();
  //Isn't it already opened?
  iterator i;
  init_iterator(opened_prefs, &i);
  struct opened* o;
  bool matched = false;
  while((o = (struct opened*)get_next(&i)) != NULL){
    if(name_match(section, o->section)){
      if(name_match(o->key, key)){
        matched = true;
        break;
      }
    }
  }

  if(matched){ //It is...
    *prf = (pref_struct*)my_malloc(sizeof(pref_struct));
    (*prf)->data = &(o->prf);
    (*prf)->cbk = cbk;
    (*prf)->param = param;
    add_element(o->prf.refs, *prf);
    return true;
  }
  
  if(get_key(section, key) == NULL){
    log_message("Attempted to open nonexistent key '%s' in section '%s'!\n",
                key, section);
    return false;
  }
  
  *prf = (pref_struct*)my_malloc(sizeof(pref_struct));
  (*prf)->cbk = cbk;
  (*prf)->param = param;

  o = (struct opened*)my_malloc(sizeof(struct opened));
  if(section != NULL){// Check it !!!
    o->prf.section_name = my_strdup(section);
  }else{
    o->prf.section_name = NULL;
  }
  o->section = o->prf.section_name;
  o->prf.key_name = my_strdup(key);
  o->key = o->prf.key_name;
  o->prf.data_type = NO_TYPE;
  o->prf.refs = create_list();
  (*prf)->data = &(o->prf);
  add_element(o->prf.refs, *prf);
  add_element(opened_prefs, o);
  return true;
}

float get_flt(pref_id pref)
{
  assert(pref != NULL);
  pref_data *prf = pref->data;
  assert(prf != NULL);
  char *section = prf->section_name;
  char *key = prf->key_name;
  if(prf->data_type == NO_TYPE){
    prf->data_type = FLT;
    prf->flt = atof(get_key(section, key));
  }
  assert(prf->data_type == FLT);
  return prf->flt;
}

int get_int(pref_id pref)
{
  assert(pref != NULL);
  pref_data *prf = pref->data;
  assert(prf != NULL);
  char *section = prf->section_name;
  char *key = prf->key_name;
  if(prf->data_type == NO_TYPE){
    prf->data_type = INT;
    prf->integer = atoi(get_key(section, key));
  }
  assert(prf->data_type == INT);
  return prf->integer;
}

char *get_str(pref_id pref)
{
  assert(pref != NULL);
  pref_data *prf = pref->data;
  assert(prf != NULL);
  char *section = prf->section_name;
  char *key = prf->key_name;
  if(prf->data_type == NO_TYPE){
    prf->data_type = STR;
    char *val = get_key(section, key);
    assert(val != NULL);
    prf->string = my_strdup(val);
  }
  assert(prf->data_type == STR);
  return prf->string;
}

void mark_pref_changed(pref_id *prf)
{
  assert(prf != NULL);
  iterator i;
  init_iterator((*prf)->data->refs, &i);
  
  pref_struct* ref;
  while((ref = (pref_struct*)get_next(&i)) != NULL){
    if(ref->cbk != NULL){
      (ref->cbk)(ref->param);
    }
  }
}

bool set_flt(pref_id *pref, float f)
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
  bool rv = change_key(section, key, new_val);
  free(new_val);
  if(rv == true){
    prf->flt = f;
    mark_pref_changed(pref);
  }
  return rv;
}

bool set_int(pref_id *pref, int i)
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
  bool rv = change_key(section, key, new_val);
  free(new_val);
  if(rv == true){
    prf->integer = i;
    mark_pref_changed(pref);
  }
  return rv;
}

bool set_str(pref_id *pref, char *str)
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
  bool rv = change_key(section, key, str);
  if(rv == true){
    if(has_payload){
      free(prf->string);
    }
    prf->string = my_strdup(str);
    mark_pref_changed(pref);
  }
  return rv;
}


bool close_pref(pref_id *pref)
{
  assert(pref != NULL);
  pref_data *prf = (*pref)->data;
  assert(prf != NULL);
  //print_opened();
  //Find pref in the list of opened prefs
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
  
  assert(matched);
  
  iterator j;
  init_iterator(o->prf.refs, &j);
  pref_struct *p;
  matched = false;
  while((p = (pref_struct*)get_next(&j)) != NULL){
    if(p == *pref){
      matched = true;
      break;
    }
  }
  
  assert(matched);
  
  delete_current(o->prf.refs, &j);
  
  if(is_empty(o->prf.refs)){
    free_list(o->prf.refs, false);
    if(prf->section_name != NULL){
      free(prf->section_name);
      prf->section_name = NULL;
    }
    free(prf->key_name);
    prf->key_name = NULL;
    if(prf->data_type == STR){
      free(prf->string);
    }
    o = (struct opened*)delete_current(opened_prefs, &i);
    assert(o != NULL);
    free(o);
  }
  if(is_empty(opened_prefs)){
    free_list(opened_prefs, false);
    opened_prefs = NULL;
  }
  free(*pref);
  *pref = NULL;
  return true;
}



bool parse_prefs(char *fname)
{
  prefs = create_list();
  if((yyin=fopen(fname, "r")) != NULL){
    parsed_file = my_strdup(fname);
    line_num = 1;
    current_section = NULL;
    //yydebug=1;
    int res = yyparse();
    fclose(yyin);
    free(parsed_file);
    parsed_file = NULL;
    if(res == 0){
      log_message("Preferences read OK!\n");
      return(true);
    }
  }
  log_message("Error encountered while reading preferences!\n");
  free_prefs();
  return(false);
}

void yyerror(char const *s)
{
  log_message("%s in file %s, line %d near '%s'\n",
  		 s, parsed_file, line_num, yytext);
}

char *get_default_file_name()
{
  char *home = getenv("HOME");
  if(home == NULL){
    log_message("Please set HOME variable!\n");
    return NULL;
  }
  char *pref_path = (char *)my_malloc(strlen(home) 
                    + strlen(pref_file) + 2);
  sprintf(pref_path, "%s/%s", home, pref_file);
  return pref_path;
}

bool read_prefs(char *file, bool force_read)
{
  static bool prefs_ok = false;
  bool free_str = false;
  
  if((prefs_read_already == false) || (force_read == true)){
    char *pfile;
    if(file != NULL){
      pfile = file;
    }else{
      pfile = get_default_file_name();
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

bool new_prefs()
{
  if(prefs_read_already){
    free_prefs();
  }
  prefs = create_list();
  prefs_read_already = true;
  return true;
}

section_struct *find_section(char *section_name)
{
  assert(prefs_read_already);
  assert(section_name != NULL);
  
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


void get_section_list(char **names[])
{
  assert(prefs_read_already);
  assert(names != NULL);
  
  plist sections = create_list();
  iterator i;
  init_iterator(prefs, &i);
  
  pref_file_item *pfi;
  while((pfi = (pref_file_item *)get_next(&i)) != NULL){
    if(pfi->item_type == SECTION){
      assert(pfi->section != NULL);
      add_element(sections, my_strdup(pfi->section->name));
    }
  }
  list2string_list(sections, names);
  return;
}

key_val_struct *find_key(char *section_name, char *key_name)
{
  assert(prefs_read_already);
  assert(section_name != NULL);
  assert(key_name != NULL);
  
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
  assert(prefs_read_already);
  if(find_section(section_name) != NULL){
    return true;
  }else{
    return false;
  }
}

bool key_exists(char *section_name, char *key_name)
{
  assert(prefs_read_already);
  if(find_key(section_name, key_name) != NULL){
    return true;
  }else{
    return false;
  }
}

char *get_key(char *section_name, char *key_name)
{
  assert(prefs_read_already);
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
  assert(prefs_read_already);
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
  assert(prefs_read_already);
  assert(key_name != NULL);
  assert(new_value != NULL);
  
  if(section_name == NULL){
    assert(custom_section_name != NULL);
    section_name = custom_section_name;
  }
  key_val_struct *kv = NULL;
  kv = find_key(section_name, key_name);
  if(kv == NULL){
    return add_key(section_name, key_name, new_value);
  }else{
    free(kv->value);
    kv->value = my_strdup(new_value);
  }
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
    of = stdout;
  }else{
    of = fopen(file_name, "w");
    if(of == NULL){
      log_message("Can't open file '%s'!\n", file_name);
      return false;
    }
  }
  assert(prefs_read_already);
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
      default:
	assert(0);
	break;
    }
    free(sci);
  }
  free_list(section->contents, false);
}

void free_prefs()
{
  iterator i;
  assert(prefs != NULL);
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
      default:
	assert(0);
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
  char *pfile = get_default_file_name();
  if(pfile == NULL){
    log_message("Can't find preference file!\n");
    return false;
  }
  char *tmp_file = NULL;
  char *old_file = NULL;
  asprintf(&tmp_file, "%s.new", pfile);
  if(dump_prefs(tmp_file) == true){
    asprintf(&old_file, "%s.old", pfile);
    remove(old_file);
    if(rename(pfile, old_file) != 0){
      log_message("Can't rename '%s' to '%s'\n", old_file, pfile);
    }
    if(rename(tmp_file, pfile) != 0){
      log_message("Can't rename '%s' to '%s'\n", tmp_file, pfile);
      free(tmp_file);
      free(old_file);
      free(pfile);
      return false;
    }
  }else{
    log_message("Can't write prefs to file '%s'\n", tmp_file);
    free(tmp_file);
      free(pfile);
    return false;
  }
  if(tmp_file != NULL) free(tmp_file);
  if(old_file != NULL) free(old_file);
  free(pfile);
  return true;
}

bool add_section(char *name)
{
  section_struct *new_section = (section_struct*)my_malloc(sizeof(section_struct));
  new_section->name = my_strdup(name);
  new_section->contents = create_list();
  pref_file_item *item = 
    (pref_file_item*)my_malloc(sizeof(pref_file_item));
  item->item_type = SECTION;
  item->section = new_section;
  add_element(prefs, item);
  return true;
}