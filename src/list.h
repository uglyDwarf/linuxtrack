#ifndef LIST__H
#define LIST__H

#include <stdbool.h>
#include "utils.h"

typedef struct list_element* iterator;
typedef struct list* plist;

/*
 * Returns pointer empty list.
 * When you don't need it, remember to free it!
 */
plist create_list();

/*
 * Returns true if list is not empty
 */
bool is_empty(plist l);

/*
 * Adds new element (payload) to the list. 
 */
void add_element(plist pl, void *payload);

/*
 * Initializes iterator i and points it to the 
 * begining of the list 
 */
void init_iterator(plist l, iterator *i);

/*
 * Returns pointer to next element in the list, 
 * NULL if you've reached the end...
 */
void* get_next(iterator* i);

/*
 * Returns pointer to current element, NULL if at
 * the end
 */
void* get_current(iterator* i);

void *delete_current(plist pl, iterator* i);

/*
 * Frees the list and optionaly also its elements
 */
void free_list(plist list_ptr, bool free_payload);

#endif


/*
  Usage example:
  --------------
  
  plist l = create_list();
  
  add_element(l, strdup("a'));
  add_element(l, strdup("b"));
  add_element(l, strdup("c"));
  
  iterator i;
  init_iterator(l, &i);
  
  char* str;
  while((str = (char *)get_next(&i)) != NULL){
    printf("%s",str);
  }
  printf("\n");
  
  free_list(l, TRUE);
  
*/
