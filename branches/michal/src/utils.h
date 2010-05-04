#ifndef UTILS__H
#define UTILS__H

#include <stdlib.h>
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif


void* my_malloc(size_t size);
char* my_strdup(const char* s);
void log_message(const char *format, ...);
int my_ioctl(int d, int request, void *argp);
void strlower(char *s);
char *my_strcat(const char *str1, const char *str2);

/*
 * Creates array of strings out of list of strings.
 * Counts on the fact that list elements are strings
 * The list is freed at the end (just the list, the array inherits those strings)
 */
int list2string_list(plist l, char **ids[]);
void array_cleanup(char **ids[]);

#ifdef __cplusplus
}
#endif

#endif
