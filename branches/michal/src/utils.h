#ifndef UTILS__H
#define UTILS__H

#include <stdlib.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


void* ltr_int_my_malloc(size_t size);
char* ltr_int_my_strdup(const char* s);
void ltr_int_log_message(const char *format, ...);
void ltr_int_valog_message(const char *format, va_list va);
int ltr_int_my_ioctl(int d, int request, void *argp);
void ltr_int_strlower(char *s);
char *ltr_int_my_strcat(const char *str1, const char *str2);

#ifdef __cplusplus
}
#endif

#endif
