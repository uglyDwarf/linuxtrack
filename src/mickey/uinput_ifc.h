#ifndef UINPUT_IFC__H
#define UINPUT_IFC__H

#include <stdbool.h>

enum buttons{LEFT_BUTTON = 1, RIGHT_BUTTON = 2, MIDDLE_BUTTON = 4};

#ifdef __cplusplus
extern "C" {
#endif

int open_uinput(char **fname, bool *permProblem);
bool create_device(int fd);
void movem(int fd, int dx, int dy);
void click(int fd, int btns);
void close_uinput(int fd);

#ifdef __cplusplus
}
#endif

#endif