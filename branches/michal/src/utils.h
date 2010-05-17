#ifndef UTILS__H
#define UTILS__H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


void* my_malloc(size_t size);
char* my_strdup(const char* s);
void log_message(const char *format, ...);
int my_ioctl(int d, int request, void *argp);
void strlower(char *s);
char *my_strcat(const char *str1, const char *str2);

#ifdef __cplusplus
}
#endif

#endif
