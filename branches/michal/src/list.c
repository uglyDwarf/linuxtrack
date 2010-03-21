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

void init_iterator(plist l, iterator *i)
{
  assert(l != NULL);
  assert(i != NULL);
  i->parent = l;
  i->elem = l->head;
}

void init_rev_iterator(plist l, iterator *i)
{
  assert(l != NULL);
  assert(i != NULL);
  i->parent = l;
  i->elem = l->tail;
}

void* get_next(iterator* i)
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

void* get_prev(iterator* i)
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

void* get_current(iterator* i)
{
  assert(i != NULL);
  assert(i->parent != NULL);
  if((i->elem) == NULL){
    return(NULL);
  }
  void* payload = i->elem->payload;
  return(payload);
}

void *delete_current(plist pl, iterator* i)
{
  assert(i != NULL);
  assert(i->parent != NULL);
  assert(i->parent == pl);
  if(is_empty(pl)){
    log_message("Attempted to delete from empty list!\n");
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
    log_message("Can't deternime which element to delete!\n");
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

void free_list(plist list_ptr, bool free_payload)
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

