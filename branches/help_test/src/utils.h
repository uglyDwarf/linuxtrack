#ifndef UTILS__H
#define UTILS__H

#include <stdlib.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIBLINUXTRACK_SRC
  #define LIBLINUXTRACK_PRIVATE
#else
  #define LIBLINUXTRACK_PRIVATE static
#endif

LIBLINUXTRACK_PRIVATE void* ltr_int_my_malloc(size_t size);
LIBLINUXTRACK_PRIVATE char* ltr_int_my_strdup(const char* s);
void ltr_int_set_logfile_name(const char *fname);
LIBLINUXTRACK_PRIVATE void ltr_int_log_message(const char *format, ...);
LIBLINUXTRACK_PRIVATE void ltr_int_valog_message(const char *format, va_list va);
int ltr_int_my_ioctl(int d, int request, void *argp);
void ltr_int_strlower(char *s);
LIBLINUXTRACK_PRIVATE char *ltr_int_my_strcat(const char *str1, const char *str2);
LIBLINUXTRACK_PRIVATE char *ltr_int_get_default_file_name(char *fname);
LIBLINUXTRACK_PRIVATE char *ltr_int_get_app_path(const char *suffix);
char *ltr_int_get_data_path(const char *data);
char *ltr_int_get_lib_path(const char *libname);
char *ltr_int_get_resource_path(const char *section, const char *rsrc);

#ifdef __cplusplus
}
#endif

#endif
