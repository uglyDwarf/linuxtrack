#ifndef LTR_SRV_LAVE__H
#define LTR_SRV_LAVE__H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//int open_slave_fifo(int master_fifo, const char *name_template, int max_fifos);
//void try_start_master(const char *main_fifo);
//void *slave_reader_thread(void *param);
bool slave(const char *profile);

#ifdef __cplusplus
}
#endif

#endif
