#ifndef LIST__H
#define LIST__H

#include <stdbool.h>
#include "utils.h"

typedef struct list* plist;
typedef struct iterator{
  struct list_element *elem;
  plist parent;
} iterator;

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
 * Initializes iterator i and points it to the 
 * end of the list 
 */
void init_rev_iterator(plist l, iterator *i);

/*
 * Returns pointer to next element in the list, 
 * NULL if you've reached the end...
 */
void* get_next(iterator* i);

/*
 * Returns pointer to previous element in the list, 
 * NULL if you've reached the beginning...
 */
void* get_prev(iterator* i);

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

