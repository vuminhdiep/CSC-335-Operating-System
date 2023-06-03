#include <linked_list.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>

int TESTNUM = 0;

Linked_List *linked_list_create(void)
{
  Linked_List * ptr = kmalloc(sizeof(Linked_List));
  ptr -> length = 0;
  ptr -> first = NULL;
  ptr -> last = NULL;
	ptr->lock = kmalloc(sizeof(lock));
  ptr->lock = lock_create("linked-list-lock");

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
  lock_acquire(list->lock);
  Linked_List_Node * newnode;
  Linked_List_Node * f = list -> first;
  if (list -> first == NULL) {
    linked_list_thread_yield(0);
    newnode = linked_list_create_node(0, data); //WARNING: possible bad interleavings
    list -> first = newnode;
    list -> last = newnode;
  } else {
    newnode = linked_list_create_node(f -> key - 1, data);

    newnode -> next = list -> first;
    linked_list_thread_yield(1);
    f -> prev = newnode; //WARNING: possible bad interleavings
    list -> first = newnode; //WARNING: possible bad interleavings
  }
  list -> length ++;
  lock_release(list->lock);
}

void linked_list_printlist(Linked_List *list, int which)
{
  lock_acquire(list->lock);
  Linked_List_Node *runner = list -> first;
  kprintf("%d: ", which);

  while (runner != NULL) {
    linked_list_thread_yield(4);
    kprintf("%d[%c] ", runner -> key, *((int *)runner -> data));
    runner = runner -> next;
  }

  kprintf("\n");
  lock_release(list->lock);
}

void linked_list_insert(Linked_List *list, int key, void *data)
{
  lock_acquire(list->lock);
  Linked_List_Node *runner = list->first;
  // Traverse the list to find the correct position to insert the new node
  while (runner != NULL && runner->key < key) {
    runner = runner->next;

  }
  linked_list_thread_yield(3);
  // Create the new node and insert it into the list
  Linked_List_Node *newnode = linked_list_create_node(key, data); //WARNING: possible bad interleavings

  if (runner == NULL) {
    // Insert the new node at the end of the list
    newnode->prev = list->last;

    if (list->last != NULL) {
      list->last->next = newnode;

    } else {
      list->first = newnode;

    }

    list->last = newnode;

  } else {

    // Insert the new node before the current node
    newnode->prev = runner->prev;
    newnode->next = runner;

    if (runner->prev != NULL) {
      runner->prev->next = newnode;

    } else {

      list->first = newnode;

    }

    runner->prev = newnode;

  }

  list->length++;
  lock_release(list->lock);
}

void *linked_list_remove_head(Linked_List *list, int *key)
{
  lock_acquire(list->lock);
  void *data;
  Linked_List_Node *head = list -> first;
  linked_list_thread_yield(2);
  //List is empty
  if (head == NULL){ //WARNING: possible bad interleavings
    lock_release(list->lock); //Need to release the lock before returning
    return NULL;
  }

  //Remove first node from the list
  if (head == list -> last){
    list -> last = NULL;
  } else {
    head -> next -> prev = NULL;
  }

  //Update the first node of the list
  linked_list_thread_yield(5);
  list -> first = head -> next; //WARNING: possible bad interleavings

  //Set the key parameter to the key of removed head
  *key = head -> key;

  //Return the data of the removed head
  data = head -> data;

  list -> length --;

  kfree(head);
  lock_release(list->lock);

  return data;
}

void linked_list_thread_yield(int testnum){
  if (TESTNUM == testnum){
    thread_yield();
  }
  
}
