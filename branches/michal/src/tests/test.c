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
  plist l = ltr_int_create_list();
  assert(ltr_int_is_empty(l));
  ltr_int_add_element(l, ltr_int_my_strdup("A"));
  ltr_int_add_element(l, ltr_int_my_strdup("B"));
  ltr_int_add_element(l, ltr_int_my_strdup("C"));
  return l;
}

void utils_test()
{
  char text[] = "DEADBEEF1";
  char ltext[] = "deadbeef1";
  char *ntext = NULL;
  
  ntext = ltr_int_my_malloc(1024);
  free(ntext);
  ntext = NULL;
  
  ntext = ltr_int_my_strdup(text);
  assert(strcmp(text, ntext) == 0);
  assert(text != ntext);
  
  ltr_int_strlower(ntext);
  assert(strcmp(ntext, ltext) == 0);
  assert(ntext != ltext);
  free(ntext);
  ntext = NULL;
  
  ntext = ltr_int_my_strcat(text, ltext);
  assert(strcmp(ntext, "DEADBEEF1deadbeef1") == 0);
  free(ntext);
  ntext = NULL;
  
  char **arr;
  plist l = ltr_int_create_list();
  assert(ltr_int_list2string_list(l, &arr) == 0);
  ltr_int_array_cleanup(&arr);
  
  l = make_list();
  
  assert(ltr_int_list2string_list(l, &arr) == 3);
  assert(strcmp(arr[0], "A") == 0);
  assert(strcmp(arr[1], "B") == 0);
  assert(strcmp(arr[2], "C") == 0);
  assert(arr[3] == NULL);
  ltr_int_array_cleanup(&arr);
  
  l = make_list();
  iterator ri;
  char *tmp;
  ltr_int_init_rev_iterator(l, &ri);
  assert(strcmp((char *)ltr_int_get_prev(&ri), "C") == 0);
  assert(strcmp((char *)ltr_int_get_prev(&ri), "B") == 0);
  assert(strcmp((char *)ltr_int_get_current(&ri), "A") == 0);
  assert(strcmp((char *)ltr_int_get_prev(&ri), "A") == 0);
  assert(ltr_int_get_prev(&ri) == NULL);
  assert(ltr_int_get_current(&ri) == NULL);
  
  ltr_int_init_iterator(l, &ri);
  assert(strcmp((char *)ltr_int_get_next(&ri), "A") == 0);
  assert(strcmp((char *)ltr_int_get_next(&ri), "B") == 0);
  assert(strcmp((char *)ltr_int_get_next(&ri), "C") == 0);
  assert(ltr_int_get_next(&ri) == NULL);
  
  ltr_int_init_iterator(l, &ri);
  assert(strcmp((char *)ltr_int_get_next(&ri), "A") == 0);
  assert(strcmp(tmp = (char *)ltr_int_delete_current(l, &ri), "A") == 0);
  free(tmp);
  assert(strcmp((char *)ltr_int_get_next(&ri), "B") == 0);
  assert(strcmp((char *)ltr_int_get_next(&ri), "C") == 0);
  assert(strcmp(tmp = (char *)ltr_int_delete_current(l, &ri), "C") == 0);
  free(tmp);

  ltr_int_init_iterator(l, &ri);
  assert(strcmp(tmp = (char *)ltr_int_delete_current(l, &ri), "B") == 0);
  free(tmp);
  assert(ltr_int_delete_current(l, &ri) == NULL);

  ltr_int_free_list(l, true);
  l = make_list();
  ltr_int_free_list(l, true);
  
  ltr_int_log_message("Hello!\n");
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
  
  assert(!ltr_int_read_prefs(preffile, false));
  assert((f = fopen(preffile, "w")) != NULL);
  fprintf(f, "#blabla\n");
  fprintf(f, "[Sec1]\n");
  fprintf(f, "#blablabla\n");
  fprintf(f, "Title = Sec1\n");
  fprintf(f, "a = 10\n");
  fprintf(f, "b = 1.1\n");
  fprintf(f, "c = aaa\n");
  fclose(f);
  assert(ltr_int_read_prefs(preffile, false));
  
  pref_id a, b;
  assert(ltr_int_open_pref("sec1", "a", &a));
  assert(ltr_int_get_int(a) == 10);
  ltr_int_close_pref(&a);
  assert(ltr_int_open_pref("sec1", "b", &a));
  assert(fabs(ltr_int_get_flt(a) - 1.1) < 1e-5);
  ltr_int_close_pref(&a);
  assert(ltr_int_open_pref("sec1", "c", &a));
  assert(strcmp(ltr_int_get_str(a), "aaa") == 0);
  ltr_int_close_pref(&a);
  
  assert(ltr_int_add_section("Sec2"));
  assert(ltr_int_add_key("Sec2", "x", "11"));
  assert(ltr_int_add_key("Sec2", "y", "1.2"));
  assert(ltr_int_add_key("Sec2", "z", "bbb"));
  assert(ltr_int_add_key("Sec2", "Title", "Sec2"));
  
  assert(ltr_int_open_pref("sec2", "x", &a));
  assert(ltr_int_set_int(&a, 21));
  assert(ltr_int_get_int(a) == 21);
  ltr_int_close_pref(&a);
  
  assert(ltr_int_open_pref("sec2", "y", &a));
  assert(ltr_int_set_flt(&a, 1.3));
  assert(fabs(ltr_int_get_flt(a) - 1.3) < 1e-5);
  ltr_int_close_pref(&a);
  
  assert(ltr_int_open_pref("sec2", "z", &a));
  assert(ltr_int_set_str(&a, "ccc"));
  assert(strcmp(ltr_int_get_str(a), "ccc") == 0);
  
  assert(ltr_int_open_pref("sec2", "z", &b));
  assert(ltr_int_set_str(&b, "ddd"));
  assert(strcmp(ltr_int_get_str(a), "ddd") == 0);
  ltr_int_close_pref(&b);
  
  ltr_int_dump_prefs(NULL);
  assert(!ltr_int_set_custom_section("Sec3"));
  assert(ltr_int_set_custom_section("Sec2"));
  assert(ltr_int_open_pref(NULL, "y", &b));
  assert(fabs(ltr_int_get_flt(b) - 1.3) < 1e-5);
  assert(ltr_int_set_flt(&b, 1.4));
  assert(fabs(ltr_int_get_flt(b) - 1.4) < 1e-5);
  ltr_int_print_opened();
  ltr_int_close_pref(&b);
  ltr_int_close_pref(&a);
  assert(!ltr_int_open_pref(NULL, "u", &a));
  assert(ltr_int_set_custom_section(NULL));
  
  char **arr;
  ltr_int_get_section_list(&arr);
  assert(strcmp(arr[0], "Sec1") == 0);
  assert(strcmp(arr[1], "Sec2") == 0);
  assert(arr[2] == NULL);
  ltr_int_array_cleanup(&arr);
  assert(ltr_int_section_exists("Sec1"));
  assert(!ltr_int_section_exists("Sec3"));
  assert(ltr_int_key_exists("Sec1", "a"));
  assert(!ltr_int_key_exists("Sec1", "w"));
  ltr_int_save_prefs();
  ltr_int_free_prefs();
  ltr_int_new_prefs();
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

