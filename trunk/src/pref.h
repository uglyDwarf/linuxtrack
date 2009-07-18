#ifndef PREF__H
#define PREF__H

bool section_exists(char *section_name);
bool key_exists(char *section_name, char *key_name);
char *get_key(char *section_name, char *key_name);


#endif
