#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "utils.h"
#include "list.h"
#include "pref.h"
#include "pref_int.h"

plist make_list()
{
  plist l = create_list();
  assert(is_empty(l));
  add_element(l, my_strdup("A"));
  add_element(l, my_strdup("B"));
  add_element(l, my_strdup("C"));
  return l;
}

void utils_test()
{
  char text[] = "DEADBEEF1";
  char ltext[] = "deadbeef1";
  char *ntext = NULL;
  
  ntext = my_malloc(1024);
  free(ntext);
  ntext = NULL;
  
  ntext = my_strdup(text);
  assert(strcmp(text, ntext) == 0);
  assert(text != ntext);
  
  strlower(ntext);
  assert(strcmp(ntext, ltext) == 0);
  assert(ntext != ltext);
  free(ntext);
  ntext = NULL;
  
  ntext = my_strcat(text, ltext);
  assert(strcmp(ntext, "DEADBEEF1deadbeef1") == 0);
  free(ntext);
  ntext = NULL;
  
  char **arr;
  plist l = create_list();
  assert(list2string_list(l, &arr) == 0);
  array_cleanup(&arr);
  
  l = make_list();
  
  assert(list2string_list(l, &arr) == 3);
  assert(strcmp(arr[0], "A") == 0);
  assert(strcmp(arr[1], "B") == 0);
  assert(strcmp(arr[2], "C") == 0);
  assert(arr[3] == NULL);
  array_cleanup(&arr);
  
  l = make_list();
  iterator ri;
  char *tmp;
  init_rev_iterator(l, &ri);
  assert(strcmp((char *)get_prev(&ri), "C") == 0);
  assert(strcmp((char *)get_prev(&ri), "B") == 0);
  assert(strcmp((char *)get_current(&ri), "A") == 0);
  assert(strcmp((char *)get_prev(&ri), "A") == 0);
  assert(get_prev(&ri) == NULL);
  assert(get_current(&ri) == NULL);
  
  init_iterator(l, &ri);
  assert(strcmp((char *)get_next(&ri), "A") == 0);
  assert(strcmp((char *)get_next(&ri), "B") == 0);
  assert(strcmp((char *)get_next(&ri), "C") == 0);
  assert(get_next(&ri) == NULL);
  
  init_iterator(l, &ri);
  assert(strcmp((char *)get_next(&ri), "A") == 0);
  assert(strcmp(tmp = (char *)delete_current(l, &ri), "A") == 0);
  free(tmp);
  assert(strcmp((char *)get_next(&ri), "B") == 0);
  assert(strcmp((char *)get_next(&ri), "C") == 0);
  assert(strcmp(tmp = (char *)delete_current(l, &ri), "C") == 0);
  free(tmp);

  init_iterator(l, &ri);
  assert(strcmp(tmp = (char *)delete_current(l, &ri), "B") == 0);
  free(tmp);
  assert(delete_current(l, &ri) == NULL);

  free_list(l, true);
  l = make_list();
  free_list(l, true);
  
  log_message("Hello!\n");
}

void pref_test()
{
  char preffile[] = "pref.txt";
  FILE *f;
  assert((f = fopen(preffile, "w")) != NULL);
  fprintf(f, "#blabla\n");
  fprintf(f, "[Sec1\n");
  fprintf(f, "a = 1\n");
  fclose(f);
  
  assert(!read_prefs(preffile, false));
  assert((f = fopen(preffile, "w")) != NULL);
  fprintf(f, "#blabla\n");
  fprintf(f, "[Sec1]\n");
  fprintf(f, "#blablabla\n");
  fprintf(f, "Title = Sec1\n");
  fprintf(f, "a = 10\n");
  fprintf(f, "b = 1.1\n");
  fprintf(f, "c = aaa\n");
  fclose(f);
  assert(read_prefs(preffile, false));
  
  pref_id a, b;
  assert(open_pref("sec1", "a", &a));
  assert(get_int(a) == 10);
  close_pref(&a);
  assert(open_pref("sec1", "b", &a));
  assert(fabs(get_flt(a) - 1.1) < 1e-5);
  close_pref(&a);
  assert(open_pref("sec1", "c", &a));
  assert(strcmp(get_str(a), "aaa") == 0);
  close_pref(&a);
  
  assert(add_section("Sec2"));
  assert(add_key("Sec2", "x", "11"));
  assert(add_key("Sec2", "y", "1.2"));
  assert(add_key("Sec2", "z", "bbb"));
  assert(add_key("Sec2", "Title", "Sec2"));
  
  assert(open_pref("sec2", "x", &a));
  assert(set_int(&a, 21));
  assert(get_int(a) == 21);
  close_pref(&a);
  
  assert(open_pref("sec2", "y", &a));
  assert(set_flt(&a, 1.3));
  assert(fabs(get_flt(a) - 1.3) < 1e-5);
  close_pref(&a);
  
  assert(open_pref("sec2", "z", &a));
  assert(set_str(&a, "ccc"));
  assert(strcmp(get_str(a), "ccc") == 0);
  
  assert(open_pref("sec2", "z", &b));
  assert(set_str(&b, "ddd"));
  assert(strcmp(get_str(a), "ddd") == 0);
  close_pref(&b);
  
  dump_prefs(NULL);
  assert(!set_custom_section("Sec3"));
  assert(set_custom_section("Sec2"));
  assert(open_pref(NULL, "y", &b));
  assert(fabs(get_flt(b) - 1.3) < 1e-5);
  assert(set_flt(&b, 1.4));
  assert(fabs(get_flt(b) - 1.4) < 1e-5);
  print_opened();
  close_pref(&b);
  close_pref(&a);
  assert(!open_pref(NULL, "u", &a));
  assert(set_custom_section(NULL));
  
  char **arr;
  get_section_list(&arr);
  assert(strcmp(arr[0], "Sec1") == 0);
  assert(strcmp(arr[1], "Sec2") == 0);
  assert(arr[2] == NULL);
  array_cleanup(&arr);
  assert(section_exists("Sec1"));
  assert(!section_exists("Sec3"));
  assert(key_exists("Sec1", "a"));
  assert(!key_exists("Sec1", "w"));
  save_prefs();
  free_prefs();
  new_prefs();
}



int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  
  utils_test();
  pref_test();
  printf("Tests passed!\n");
  return 0;
}

