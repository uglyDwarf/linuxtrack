#ifndef LIST__H
#define LIST__H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct list* plist;
typedef struct iterator{
  struct list_element *elem;
  plist parent;
} iterator;

/*
 * Returns pointer empty list.
 * When you don't need it, remember to free it!
 */
plist ltr_int_create_list();

/*
 * Returns true if list is not empty
 */
bool ltr_int_is_empty(plist l);

/*
 * Adds new element (payload) to the list. 
 */
void ltr_int_add_element(plist pl, void *payload);

/*
 * Initializes iterator i and points it to the 
 * begining of the list 
 */
void ltr_int_init_iterator(plist l, iterator *i);

/*
 * Initializes iterator i and points it to the 
 * end of the list 
 */
void ltr_int_init_rev_iterator(plist l, iterator *i);

/*
 * Returns pointer to next element in the list, 
 * NULL if you've reached the end...
 */
void* ltr_int_get_next(iterator* i);

/*
 * Returns pointer to previous element in the list, 
 * NULL if you've reached the beginning...
 */
void* ltr_int_get_prev(iterator* i);

/*
 * Returns pointer to current element, NULL if at
 * the end
 */
void* ltr_int_get_current(iterator* i);

void *ltr_int_delete_current(plist pl, iterator* i);

/*
 * Frees the list and optionaly also its elements
 */
void ltr_int_free_list(plist list_ptr, bool free_payload);

/*
 * Creates array of strings out of list of strings.
 * Counts on the fact that list elements are strings
 * The list is freed at the end (just the list, the array inherits those strings)
 */
int ltr_int_list2string_list(plist l, char **ids[]);
void ltr_int_array_cleanup(char **ids[]);

#ifdef __cplusplus
}
#endif

#endif

