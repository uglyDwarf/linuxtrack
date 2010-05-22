#include <stdio.h>
#include <pref_int.h>


int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  if(ltr_int_read_prefs(NULL, true)){
    char **sections = NULL;
    ltr_int_get_section_list(&sections);
    char *name;
    int i = 0;
    while((name = sections[i]) != NULL){
      printf("Section: %s\n", name);
      ++i;
    }
    ltr_int_array_cleanup(&sections);
    ltr_int_free_prefs();
  }
  return 0;
}
