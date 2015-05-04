#ifndef ARGS__H
#define ARGS__H

#include <stdbool.h>

bool checkCmdLine(int argc, char *argv[]);
bool doEnumCams();
bool doCapture();
bool doMap();
bool doFacetrack();
const char *getCamName();
const char *getMapFileName();
void getRes(int *x, int *y);
const char *getCascadeFileName();

#endif