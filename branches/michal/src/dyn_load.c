#include <stdlib.h>
#include <dlfcn.h>
#include "dyn_load.h"
#include "utils.h"

void *lt_load_library(char *lib_name, lib_fun_def_t *func_defs)
{
  void *libhandle;
  libhandle = dlopen(lib_name, RTLD_NOW | RTLD_LOCAL);
  if(libhandle == NULL){
    log_message("Couldn't load library '%s' - %s!\n", lib_name, dlerror());
    return NULL;
  }
  dlerror(); //clear any existing error...
  //log_message("Handle: %p\n", libhandle);
  while(func_defs->name != NULL){
    if((*(void **) (func_defs->ref) = dlsym(libhandle, func_defs->name)) == NULL){
      log_message("Error loding functions: %s\n", dlerror());
      dlclose(libhandle);
      return NULL;
    }
    //log_message("Loading func '%s' from '%s' => %p\n",
	//	func_defs->name, lib_name, *(void **)func_defs->ref);
    ++func_defs;
  }
  return libhandle;
}

int lt_unload_library(void *libhandle, lib_fun_def_t *func_defs)
{
  while(func_defs->name != NULL){
    *(void **)(func_defs->ref) = NULL;
    ++func_defs;
  }
  return dlclose(libhandle);
}

