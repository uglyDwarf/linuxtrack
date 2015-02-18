#ifndef LTR_SRV_SLAVE__H
#define LTR_SRV_SLAVE__H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ltr_int_slave(const char *c_profile, const char *c_com_file, const char *ppid_str,
                   const char *close_pipe_str, const char *notify_pipe_str);

#ifdef __cplusplus
}
#endif

#endif
