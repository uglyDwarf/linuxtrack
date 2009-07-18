#ifndef UTILS__H
#define UTILS__H

#include <stdlib.h>



void* my_malloc(size_t size);
char* my_strdup(const char* s);
void log_message(const char *format, ...);
int my_ioctl(int d, int request, void *argp);


#endif
