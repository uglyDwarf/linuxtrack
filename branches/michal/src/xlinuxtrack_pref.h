#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "utils.h"

enum pref_id {START_STOP, PAUSE};
struct pref;

char *get_pref_file_name();
void print_pref(struct pref *p);
bool read_pref(char *fname, struct pref *p);
bool save_pref(char *fname, struct pref *p);
bool is_pref_valid(struct pref *p);
struct pref *new_pref();
void reset_pref(struct pref *p);
bool set_pref(struct pref *p, enum pref_id id, int val);
int get_pref(struct pref *p, enum pref_id id);

