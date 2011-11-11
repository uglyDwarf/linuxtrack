#include <stdlib.h>
#include <dlfcn.h>
#include "dyn_load.h"
#include "utils.h"

void *ltr_int_load_library(char *lib_name, lib_fun_def_t *func_defs)
{
  void *libhandle;
  char *full_name = ltr_int_get_lib_path(lib_name);
  if(full_name == NULL){
    ltr_int_log_message("Couldn't get full name for library %s\n", lib_name);
    return NULL;
  }
  libhandle = dlopen(full_name, RTLD_NOW | RTLD_LOCAL);
  if(libhandle == NULL){
    ltr_int_log_message("Couldn't load library '%s' - %s!\n", full_name, dlerror());
    free(full_name);
    return NULL;
  }
  free(full_name);
  dlerror(); //clear any existing error...
  //log_message("Handle: %p\n", libhandle);
  while(func_defs->name != NULL){
    if((*(void **) (func_defs->ref) = dlsym(libhandle, func_defs->name)) == NULL){
      ltr_int_log_message("Error loding functions: %s\n", dlerror());
      dlclose(libhandle);
      return NULL;
    }
    //log_message("Loading func '%s' from '%s' => %p\n",
	//	func_defs->name, lib_name, *(void **)func_defs->ref);
    ++func_defs;
  }
  return libhandle;
}

int ltr_int_unload_library(void *libhandle, lib_fun_def_t *func_defs)
{
  while(func_defs->name != NULL){
    *(void **)(func_defs->ref) = NULL;
    ++func_defs;
  }
  return dlclose(libhandle);
}

