#ifndef PREF__H
#define PREF__H

#include <stdbool.h>

bool set_custom_section(char *name);

typedef struct pref_struct *pref_id;
bool open_game_pref(char *key, pref_id *prf);
float get_flt(pref_id prf);
int get_int(pref_id prf);
char *get_str(pref_id prf);
bool close_game_pref(pref_id *prf);

#endif
