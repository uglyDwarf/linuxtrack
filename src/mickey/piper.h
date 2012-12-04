#ifndef PIPER__H
#define PIPER__H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int prepareBtnChanel();
bool fetch_data(int fifo, void *data, ssize_t length, ssize_t *read);

#ifdef __cplusplus
}
#endif

#endif