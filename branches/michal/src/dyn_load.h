#ifndef DYN_LOAD__H
#define DYN_LOAD__H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
  char *name;
  void *ref;
} lib_fun_def_t;

void *lt_load_library(char *lib_name, lib_fun_def_t *func_defs);
int lt_unload_library(void *libhandle, lib_fun_def_t *func_defs);


#ifdef __cplusplus
}
#endif

#endif
