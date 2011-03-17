#include <stdbool.h>

bool initWiiCom(bool isServer, struct mmap_s **mmm_p);
void closeWiiCom();
void pauseWii();
void resumeWii();
