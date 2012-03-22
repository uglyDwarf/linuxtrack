#ifndef WIICOM__H
#define WIICOM__H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

bool ltr_int_initWiiCom(bool isServer, struct mmap_s **mmm_p);
void ltr_int_closeWiiCom();
void ltr_int_pauseWii();
void ltr_int_resumeWii();

#ifdef __cplusplus
}
#endif

#endif
