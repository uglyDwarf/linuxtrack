%{
  #include <stdio.h>
  #include <string.h>
  #include "utils.h"
  #include "list.h"
  #include "pref_int.h"
  extern FILE* yyin;
  int yylex (void);
  void yyerror (char const *);
  section_struct *current_section = NULL;
%}

%debug
%error-verbose

%union {
  char *str;
}


%token TOKEN_COMMENT
%token TOKEN_LEFT_BRACKET
%token TOKEN_RIGHT_BRACKET
%token TOKEN_KEY
%token TOKEN_EQ
%token TOKEN_VALUE
%token TOKEN_SECNAME


%type <str> TOKEN_COMMENT TOKEN_KEY TOKEN_VALUE TOKEN_SECNAME

%%
input:		/* empty */
      		| input section
      		| input TOKEN_COMMENT {
                  if(current_section == NULL){
                    pref_file_item *item = 
                      my_malloc(sizeof(pref_file_item));
                    item->item_type = PREF_COMMENT;
                    item->comment = $2;
                    add_element(prefs, item);
                  }else{
                    section_item *item = 
                      my_malloc(sizeof(section_item));
                    item->sec_item_type = SEC_COMMENT;
                    item->comment = $2;
                    add_element(current_section->contents, item);
                  }
                }
                | input key_val_pair
;

section:	TOKEN_LEFT_BRACKET TOKEN_SECNAME TOKEN_RIGHT_BRACKET {
                  current_section = my_malloc(sizeof(section_struct));
                  current_section->name = $2;
                  current_section->contents = create_list();
                  pref_file_item *item = 
                    my_malloc(sizeof(pref_file_item));
                  item->item_type = SECTION;
                  item->section = current_section;
                  add_element(prefs, item);
		}
;

key_val_pair:	TOKEN_KEY TOKEN_EQ TOKEN_VALUE {
		  key_val_struct *key_val = my_malloc(sizeof(key_val_struct));
                  key_val->key = $1;
                  key_val->value = $3;
                    section_item *item = 
                      my_malloc(sizeof(section_item));
                    item->sec_item_type = KEY_VAL;
		  item->key_val = key_val;
		  add_element(current_section->contents, item);
		}
;
%%



