#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "pref_int.h"

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
  iterator i;
  init_iterator(prefs, &i);
  
  pref_file_item *pfi;
  while((pfi = (pref_file_item *)get_next(&i)) != NULL){
    if(pfi->item_type == SECTION){
      assert(pfi->section != NULL);
      if(strcmp(pfi->section->name, section_name) == 0){
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
    return NULL;
  }
  iterator i;
  init_iterator(sec->contents, &i);
  
  section_item *si;
  while((si = (section_item *)get_next(&i)) != NULL){
    if(si->sec_item_type == KEY_VAL){
      assert(si->key_val != NULL);
      if(strcmp(si->key_val->key, key_name) == 0){
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
  key_val_struct *kv = find_key(section_name, key_name);
  if(kv == NULL){
    return NULL;
  }
  return kv->value;
}

bool change_key(char *section_name, char *key_name, char *new_value)
{
  if(read_prefs_on_init() == false){
    return false;
  }
  key_val_struct *kv = find_key(section_name, key_name);
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

char *get_custom_key(char *key_name)
{
  if((custom_section_name != NULL) && 
     (key_exists(custom_section_name, key_name) == true)){
    return get_key(custom_section_name, key_name);
  }else if(key_exists(def_section_name, key_name) == true){
    return get_key(def_section_name, key_name);
  }else{
    log_message("Attempted to get nonexistent key '%s'\n", key_name);
  }
  return NULL;
};


/*
int main(int argc, char *argv[])
{
  printf("Device type: %s\n", get_key("Global", "Capture-device"));
  printf("Head ref [0, %s, %s]\n", get_key("Global", "Head-Y"), 
  	get_key("Global", "Head-Z"));
  dump_prefs("pref1.dmp");
  change_key("Global", "Head-Y", "333");
  change_key("Global", "Head-Z", "444");
  printf("Head ref [0, %s, %s]\n", get_key("Global", "Head-Y"), 
  	get_key("Global", "Head-Z"));
  dump_prefs("pref2.dmp");
  free_prefs();
  return 0;
}
*/
