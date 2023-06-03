#include <linked_list.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>

int testnum = 0;

Linked_List *linked_list_create(void)
{
  Linked_List * ptr = kmalloc(sizeof(Linked_List));
  if (ptr == NULL){
    return NULL;
  }

  ptr->list_lock = lock_create("list lock");
  if (ptr->list_lock == NULL){
    kfree(ptr);
    return NULL;
  }
  ptr -> length = 0;
  ptr -> first = NULL;
  ptr -> last = NULL;

  return ptr;
}

Linked_List_Node *linked_list_create_node(int key, void *data)
{
  Linked_List_Node *newnode = kmalloc(sizeof(Linked_List_Node));
  newnode -> prev = NULL;
  newnode -> next = NULL;
  newnode -> key = key;
  newnode -> data = data;

  return newnode;
}

void linked_list_prepend(Linked_List *list, void *data)
{
  lock_acquire(list->list_lock);
  Linked_List_Node * newnode;
  Linked_List_Node * f = list -> first;

  if (list -> first == NULL) {
    if (testnum == 1){
      thread_yield();
    }
    newnode = linked_list_create_node(0, data);
    list -> first = newnode;
    list -> last = newnode;
  } else {
    newnode = linked_list_create_node(f -> key - 1, data);

    newnode -> next = list -> first;
    f -> prev = newnode;
    list -> first = newnode;
  }

  list -> length ++;
  lock_release(list->list_lock);
}

void linked_list_printlist(Linked_List *list, int which)
{
  lock_acquire(list->list_lock);
  Linked_List_Node *runner = list -> first;

  kprintf("%d: ", which);

  while (runner != NULL) {
    kprintf("%d[%c] ", runner -> key, *((int *)runner -> data));
    runner = runner -> next;
  }

  kprintf("\n");
  lock_release(list->list_lock);
}

// added in proj 1

void linked_list_insert(Linked_List *list, int key, void *data){
  lock_acquire(list->list_lock);
  Linked_List_Node * newnode;
  Linked_List_Node * f = list -> first;
  if (testnum == 2){
thread_yield();
}

  if (f == NULL) {
    newnode = linked_list_create_node(key, data);
    list -> first = newnode;
    list -> last = newnode;
  } else {
    newnode = linked_list_create_node(key, data);

    if (newnode -> key <= f -> key){

      newnode -> next = f;
      f -> prev = newnode;
if (testnum == 3){
thread_yield();}
      list -> first = newnode;
  
    }
    else {
    while ((f -> next != NULL) && (f -> next -> key <= newnode -> key)){
        f = f -> next;
      }
      

      newnode -> next = f -> next;
      newnode -> prev = f;
	
      f -> next = newnode;

      if (newnode -> next == NULL){


        list -> last = newnode;
        
      }
      
    }
  }

  
  
  list -> length ++;
  lock_release(list->list_lock);
}

void *linked_list_remove_head(Linked_List *list, int *key){
  lock_acquire(list->list_lock);

  if (list -> first != NULL){
  Linked_List_Node *first_ptr;
  int key_saved;
  void *data_return;

  first_ptr = list -> first;

  data_return = first_ptr -> data;

  key_saved = first_ptr -> key;
if (testnum == 4){
thread_yield();}
  list -> first = first_ptr -> next;
  

  if (list -> first != NULL){
  list -> first -> prev = NULL; // investigate this line 
  }

  list -> length --;

  *key = key_saved;

  kfree(first_ptr);

  if (list -> first == NULL){
    list -> last = NULL;
  }
  lock_release(list->list_lock);
  return data_return;

  }
  else {
    lock_release(list->list_lock);
    return NULL;
  }



}


