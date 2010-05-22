#include <assert.h>
#include "list.h"
#include "utils.h"

typedef struct list_element{
  struct list_element *prev;
  struct list_element *next;
  void *payload;
} list_element;

typedef struct list{
  list_element *head;
  list_element *tail;
} list; 


plist ltr_int_create_list()
{
  plist new_list = ltr_int_my_malloc(sizeof(list));
  new_list->head = new_list->tail = NULL;
  return(new_list);
}

bool ltr_int_is_empty(plist l)
{
  if(l->head == NULL){
    return true;
  }else{
    return false;
  }
}

void ltr_int_add_element(plist pl, void *payload)
{
  list_element* new_elem = ltr_int_my_malloc(sizeof(list_element));
  new_elem->prev = NULL;
  new_elem->next = NULL;
  new_elem->payload = payload;
  
  assert(pl != NULL);
  if(pl->head == NULL){
    // the list is empty
    pl->head = pl->tail = new_elem; //the only element
  }else{
    //add new element to the end
    assert(pl->tail != NULL);
    pl->tail->next = new_elem;
    new_elem->prev = pl->tail;
    pl->tail = new_elem;
  }
}

void ltr_int_init_iterator(plist l, iterator *i)
{
  assert(l != NULL);
  assert(i != NULL);
  i->parent = l;
  i->elem = l->head;
}

void ltr_int_init_rev_iterator(plist l, iterator *i)
{
  assert(l != NULL);
  assert(i != NULL);
  i->parent = l;
  i->elem = l->tail;
}

void* ltr_int_get_next(iterator* i)
{
  assert(i != NULL);
  assert(i->parent != NULL);
  if((i->elem) == NULL){
    return(NULL);
  }
  void* payload = i->elem->payload;
  i->elem = i->elem->next;
  return(payload);
}

void* ltr_int_get_prev(iterator* i)
{
  assert(i != NULL);
  assert(i->parent != NULL);
  if((i->elem) == NULL){
    return(NULL);
  }
  void* payload = i->elem->payload;
  i->elem = i->elem->prev;
  return(payload);
}

void* ltr_int_get_current(iterator* i)
{
  assert(i != NULL);
  assert(i->parent != NULL);
  if((i->elem) == NULL){
    return(NULL);
  }
  void* payload = i->elem->payload;
  return(payload);
}

void *ltr_int_delete_current(plist pl, iterator* i)
{
  assert(i != NULL);
  assert(i->parent != NULL);
  assert(i->parent == pl);
  if(ltr_int_is_empty(pl)){
    ltr_int_log_message("Attempted to delete from empty list!\n");
    return NULL;
  }
  //iterator is already one element farther
  list_element *current = NULL;
  if((i->elem) == NULL){
    current = pl->tail;
  }else if(i->elem == pl->head){
    current = pl->head;
  }else{
    current = i->elem->prev;
  }
  if(current == NULL){
    ltr_int_log_message("Can't determine which element to delete!\n");
    return NULL; 
  }
  
  if(pl->head == current){
    pl->head = current->next;
  }
  if(pl->tail == current){
    pl->tail = current->prev;
  }
  list_element *prev = current->prev;
  list_element *next = current->next;
  if(prev != NULL){
    prev->next = next;
  }
  if(next != NULL){
    next->prev = prev;
  }
  void *payload = current->payload;
  free(current);
  return payload;
}

void ltr_int_free_list(plist list_ptr, bool free_payload)
{
  list_element* elem;
  assert(list_ptr != NULL);
  if(list_ptr->head != NULL){
    while(list_ptr->head != NULL){
      elem = list_ptr->head;
      list_ptr->head = elem->next;
      if(free_payload == true){
	free(elem->payload);
      }
      free(elem);
    }
  }
  list_ptr->tail = NULL;
  free(list_ptr);
}

int ltr_int_list2string_list(plist l, char **ids[])
{
  int j;
  iterator i;
  char *id;
  if(!ltr_int_is_empty(l)){
    j = 1;
    ltr_int_init_iterator(l, &i);
    while((id = (char *)ltr_int_get_next(&i)) != NULL){
      ++j;
    }
  }else{
    j = 1;
  }
  *ids = (char **)ltr_int_my_malloc(j * sizeof(char *));
  j = 0;
  ltr_int_init_iterator(l, &i);
  while((id = (char *)ltr_int_get_next(&i)) != NULL){
    (*ids)[j] = id;
    ++j;
  }
  (*ids)[j] = NULL;
  ltr_int_free_list(l, false);
  return j;
}

void ltr_int_array_cleanup(char **ids[])
{
  int i = 0;
  while((*ids)[i] != NULL){
    free((*ids)[i]);
    ++i;
  }
  free(*ids);
  *ids = NULL;
  return;
}

