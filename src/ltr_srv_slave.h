#ifndef LTR_SRV_LAVE__H
#define LTR_SRV_LAVE__H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool slave(const char *profile, bool restart_master);

#ifdef __cplusplus
}
#endif

#endif
