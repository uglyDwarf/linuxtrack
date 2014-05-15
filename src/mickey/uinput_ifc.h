#ifndef UINPUT_IFC__H
#define UINPUT_IFC__H

#include <stdbool.h>
#include "mouse.h"

#ifdef __cplusplus
extern "C" {
#endif

int open_uinput(char **fname, bool *permProblem);
bool create_device(int fd);
bool movem(int fd, int dx, int dy);
bool clickm(int fd, buttons_t btns, struct timeval ts);
void close_uinput(int fd);

#ifdef __cplusplus
}
#endif

#endif
