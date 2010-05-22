#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "utils.h"

enum pref_id {START_STOP, PAUSE};
struct pref;

char *xltr_get_pref_file_name();
void xltr_print_pref(struct pref *p);
bool xltr_read_pref(char *fname, struct pref *p);
bool xltr_save_pref(char *fname, struct pref *p);
bool xltr_is_pref_valid(struct pref *p);
struct pref *xltr_new_pref();
void xltr_reset_pref(struct pref *p);
bool xltr_set_pref(struct pref *p, enum pref_id id, int val);
int xltr_get_pref(struct pref *p, enum pref_id id);

