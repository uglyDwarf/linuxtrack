#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "utils.h"
#include "list.h"
#include "pref.h"

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


int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  
  utils_test();
  printf("Tests passed!\n");
  return 0;
}

