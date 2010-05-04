#include <stdio.h>
#include <pref_int.h>


int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  if(read_prefs(NULL, true)){
    char **sections = NULL;
    get_section_list(&sections);
    char *name;
    int i = 0;
    while((name = sections[i]) != NULL){
      printf("Section: %s\n", name);
      ++i;
    }
    array_cleanup(&sections);
    free_prefs();
  }
  return 0;
}
