#include <assert.h>
#include "list.h"



typedef struct list_element{
  struct list_element *next;
  void *payload;
} list_element;

typedef struct list{
  list_element *head;
  list_element *tail;
} list; 



plist create_list()
{
  plist new_list = my_malloc(sizeof(list));
  new_list->head = new_list->tail = NULL;
  return(new_list);
}

bool is_empty(plist l)
{
  if(l->head == NULL){
    return true;
  }else{
    return false;
  }
}

void add_element(plist pl, void *payload)
{
  list_element* new_elem = my_malloc(sizeof(list_element));
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
    pl->tail = new_elem;
  }
}

void init_iterator(plist l, iterator *i)
{
  assert(l != NULL);
  assert(i != NULL);
  *i = l->head;
}

void* get_next(iterator* i)
{
  assert(i != NULL);
  if((*i) == NULL){
    return(NULL);
  }
  void* payload = (*i)->payload;
  *i = (*i)->next;
  return(payload);
}

void* get_current(iterator* i)
{
  assert(i != NULL);
  if((*i) == NULL){
    return(NULL);
  }
  void* payload = (*i)->payload;
  return(payload);
}

void free_list(plist list_ptr, bool free_payload)
{
  list_element* elem;
  assert(list_ptr != NULL);
  if(list_ptr->head == NULL){
    return;
  }
  while(list_ptr->head != NULL){
    elem = list_ptr->head;
    list_ptr->head = elem->next;
    if(free_payload == true){
      free(elem->payload);
    }
    free(elem);
  }
  list_ptr->tail = NULL;
  free(list_ptr);
}

