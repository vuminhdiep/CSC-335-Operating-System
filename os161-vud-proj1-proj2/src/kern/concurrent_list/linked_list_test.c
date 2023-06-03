#include <linked_list.h>
#include <lib.h>
#include <thread.h>
#include <spl.h>
#include <test.h>
#include <synch.h>
#include <kern/test161.h>
#include <kern/secret.h>

#define NTHREADS 2
static struct semaphore *sem = NULL;
static struct Linked_List *testls = NULL;

static void linked_list_test_adder(void *list, unsigned long which)
{
  //splhigh();
  testls = (Linked_List *) list;
  
  int i;
  int *c;
  for (i = 0; i < 10; i++) {
    c = kmalloc(sizeof(int));
    *c = 'A' + i;
    linked_list_prepend(testls, c);
    linked_list_printlist(testls, which);
  }
  V(sem);
}

static void llt_0(){
  int i, result;
  testls = linked_list_create();
  for (i=0; i<NTHREADS; i++){
    result = thread_fork("linked_list_prepend thread", NULL, linked_list_test_adder, testls, i+1);
    if (result){
      panic("linked-list-test-synch: thread_fork failed %s\n", strerror(result));
    }

  }
}

static void llt1_adder(void *list, unsigned long which)
{
  (void)which;
  int *data = kmalloc(sizeof(int));
  *data = 5;
  testls = (Linked_List *) list;
  linked_list_prepend(testls, data);
  linked_list_printlist(testls, which);
  V(sem);
}

static void llt_1(){
  int i, result;
  testls = linked_list_create();
  for (i=0; i<NTHREADS; i++){
    result = thread_fork("linked_list_prepend thread", NULL, llt1_adder, testls, i+1);
    if (result){
      panic("linked-list-test-synch: thread_fork failed %s\n", strerror(result));
    }

  }
}

static void llt2_adder(void *list, unsigned long which)
{   

    int key;
    testls = (Linked_List *) list;
    linked_list_remove_head(testls, &key);
    linked_list_printlist(testls, which);
    V(sem);
}

static void llt_2(){
  int i, result;
  testls = linked_list_create();
  for (i=0; i<NTHREADS; i++){
    result = thread_fork("linked_list_remove_head thread", NULL, llt2_adder, testls, i+1);
    if (result){
      panic("linked-list-test-synch: thread_fork failed %s\n", strerror(result));
    }

  }

}

static void llt3_adder(void *list, unsigned long which)
{ 
  //(void) which;
  testls = (Linked_List *) list;
  int i;
  int *c;
  for (i = 0; i < 10; i++) {
    c = kmalloc(sizeof(int));
    *c = 'A' + i;
    linked_list_insert(list, i, c);
    linked_list_printlist(list, which);
  }
  V(sem);

}

static void llt_3(){
  int i, result;
  testls = linked_list_create();
  for (i=0; i<NTHREADS; i++){
    result = thread_fork("linked_list_insert thread", NULL, llt3_adder, testls, i+1);
    if (result){
      panic("linked-list-test-synch: thread_fork failed %s\n", strerror(result));
    }

  }

}

static void llt4_adder(void *list, unsigned long which)
{
  testls = (Linked_List *) list;
  int i;
  int *c;
  for (i = 0; i < 10; i++) {
    c = kmalloc(sizeof(int));
    *c = 'A' + i;
    linked_list_insert(list, 0, c);
    
  }
  linked_list_printlist(testls, which);
  V(sem);
}

static void llt5_adder(void *list, unsigned long which)
{  
  //(void) which;
  int *data = kmalloc(sizeof(int));
  *data = 5;
  testls = (Linked_List *) list;
  linked_list_insert(testls, 0, data);
  linked_list_printlist(testls, which);
  
  V(sem);
}

static void llt_4(){
  
  testls = linked_list_create();
  thread_fork("linked_list_printlist thread", NULL, llt4_adder, testls, 1);
  thread_fork("linked_list_printlist thread", NULL, llt2_adder, testls, 2);


}


static void llt_5()
{
  testls = linked_list_create();
  thread_fork("linked_list_printlist thread", NULL, llt2_adder, testls, 1);
  thread_fork("linked_list_printlist thread", NULL, llt5_adder, testls, 2);


}



int linked_list_test_run(int nargs, char **args) //print correct linked list output but stuck in loop and not return to kernel menu
{
  //int result;
  int testnum = 0;
  if (nargs == 2) {
    testnum = args[1][0] - '0'; // XXX - Hack - only works for testnum 0 -- 9
  }

  kprintf("testnum: %d\n", testnum);
  int i;
  TESTNUM = testnum;

  sem = sem_create("linked-list-test-synch", 0);

  if(sem == NULL){
    panic("linked-list-test-synch: sem_create failed\n");
  }

  kprintf("Starting linked-list-test-synch:...\n");

  if (TESTNUM == 1){
    llt_1();
  } else if (TESTNUM == 2){
    llt_2();
  } else if (TESTNUM == 3) {
    llt_3();
  } else if (TESTNUM == 4){
    llt_4();
  } else if (TESTNUM == 5) {
    llt_5();
  } else {
    llt_0();
  }

  for (i=0; i<NTHREADS; i++){
    P(sem);
  }

  sem_destroy(sem);
  sem = NULL;
  kprintf_t("\n");
  success(TEST161_SUCCESS, SECRET, "linked-list-test-synch");


  // thread_fork("adder 1",
	//       NULL,
	//       linked_list_test_adder,
	//       list,
	//       1);

  // thread_fork("adder 2",
	//       NULL,
	//       linked_list_test_adder,
	//       list,
	//       2);

  // XXX - Bug - We're returning from this function without waiting
  // for these two threads to finish.  The execution of these
  // threads may interleave with the kernel's main menu thread and
  // cause interleaving of console output.  We going to accept this
  // problem for the moment until we learn how to fix in Project 2.
  // An enterprising student might investigate why this is not a
  // problem with other tests suites the kernel uses.

  return 0;
}

int custom_linked_list_test(int nargs, char **args)
{
  int testnum = 0;

  if (nargs == 2) {
    testnum = args[1][0] - '0'; // XXX - Hack - only works for testnum 0 -- 9
  }

  const int INT_MIN = -2147483648;
  const int INT_MAX = 2147483647;

  kprintf("Starting custom linked list test with testnum: %d\n", testnum);  

  Linked_List *list = linked_list_create();
  KASSERT(list->length == 0);
  KASSERT(list != NULL);
  KASSERT(list->first == NULL);
  KASSERT(list->last == NULL);

  Linked_List *another_list = linked_list_create();
  KASSERT(list != another_list);

  //Test case for linked_list_prepend()
  Linked_List * list1 = linked_list_create();
  int *data1 = kmalloc(sizeof(int));
  *data1 = 5;
  linked_list_prepend(list1, data1);
  KASSERT(list1->length == 1);
  KASSERT(list1->first != NULL);
  KASSERT(list1->last != NULL);
  KASSERT(list1->first == list1->last);
  KASSERT(list1->first->key == 0);
  KASSERT(list1->first->data == data1);
  KASSERT(list1->first->prev == NULL);
  KASSERT(list1->first->next == NULL);

  int *another_data = kmalloc(sizeof(int));
  *another_data = 15;
  linked_list_prepend(list1, another_data);
  KASSERT(list1->length == 2);
  KASSERT(list1->first->key == -1);
  KASSERT(list1->last->key == 0);

  //Test case for linked_list_create_node()
  Linked_List_Node *node1 = linked_list_create_node(1, data1);
  KASSERT(node1 != NULL);
  KASSERT(node1->key == 1);
  KASSERT(node1->data == data1);
  KASSERT(node1->prev == NULL);
  KASSERT(node1->next == NULL);

  Linked_List_Node *node2 = linked_list_create_node(1, data1);
  KASSERT(node1 != node2);
  KASSERT(node1->key == node2->key);
  KASSERT(node1->data == node2->data);
  KASSERT(node2->prev == NULL);
  KASSERT(node2->next == NULL);

  //Test case for linked_list_insert()
  Linked_List * list2 = linked_list_create();
  int *data2 = kmalloc(sizeof(int));
  *data2 = 13;
  linked_list_insert(list2, 0, data2);
  KASSERT(list2->length == 1);
  KASSERT(list2->first != NULL);
  KASSERT(list2->last != NULL);
  KASSERT(list2->first == list2->last);
  KASSERT(list2->first->key == 0);
  KASSERT(list2->first->data == data2);
  KASSERT(list2->first->prev == NULL);
  KASSERT(list2->first->next == NULL);

  int *data3 = kmalloc(sizeof(int));
  *data3 = 100;
  linked_list_insert(list2, 1, data3);
  KASSERT(list2->length == 2);
  KASSERT(list2->first != NULL);
  KASSERT(list2->last != NULL);
  KASSERT(list2->first != list2->last);
  KASSERT(list2->first->key == 0);
  KASSERT(list2->first->data == data2);
  KASSERT(list2->first->prev == NULL);
  KASSERT(list2->first->next != NULL);
  KASSERT(list2->first->next->key == 1);
  KASSERT(list2->first->next->data == data3);
  KASSERT(list2->first->next->prev == list2->first);
  KASSERT(list2->first->next->next == NULL);
  KASSERT(list2->last->key == 1);
  KASSERT(list2->last->data == data3);
  KASSERT(list2->last->prev == list2->first);
  KASSERT(list2->last->next == NULL);

  int *data4 = kmalloc(sizeof(int));
  *data4 = -999;
  linked_list_insert(list2, 2, data4);
  KASSERT(list2->length == 3);
  KASSERT(list2->first != NULL);
  KASSERT(list2->last != NULL);
  KASSERT(list2->first != list2->last);
  KASSERT(list2->first->key == 0);
  KASSERT(list2->first->data == data2);
  KASSERT(list2->first->prev == NULL);
  KASSERT(list2->first->next != NULL);
  KASSERT(list2->first->next->key == 1);
  KASSERT(list2->first->next->data == data3);
  KASSERT(list2->first->next->prev == list2->first);
  KASSERT(list2->first->next->next != NULL);
  KASSERT(list2->first->next->next->data == data4);
  KASSERT(list2->first->next->next->prev == list2->first->next);
  KASSERT(list2->first->next->next->next == NULL);
  KASSERT(list2->last->key == 2);
  KASSERT(list2->last->data == data4);
  KASSERT(list2->last->prev == list2->first->next);
  KASSERT(list2->last->next == NULL);

  //Test case for linked_list_remove_head()
  Linked_List *list3 = linked_list_create();
  int key;
  void *data_out7 = linked_list_remove_head(list3, &key);
  KASSERT(data_out7 == NULL);
  KASSERT(list3->length == 0);
  KASSERT(list3->first == NULL);
  KASSERT(list3->last == NULL);
  
  int *data8 = kmalloc(sizeof(int));
  *data8 = 10;
  linked_list_prepend(list3, data8);
  void *data_out9 = linked_list_remove_head(list3, &key);
  KASSERT(data_out9 == data8);
  KASSERT(key == 0);
  KASSERT(list3->length == 0);
  KASSERT(list3->first == NULL);
  KASSERT(list3->last == NULL);

  int *data10 = kmalloc(sizeof(int));
  *data10 = -1;
  linked_list_prepend(list3, data10);

  int *data11 = kmalloc(sizeof(int));
  *data11 = 1;
  linked_list_prepend(list3, data11);

  int *data12 = kmalloc(sizeof(int));
  *data12 = -99;
  linked_list_prepend(list3, data12);

  KASSERT(list3->length == 3);
  KASSERT(list3->first->data == data12);
  KASSERT(list3->last->data == data10);
  KASSERT(list3->first->key == -2);

  void *data_out13 = linked_list_remove_head(list3, &key);
  KASSERT(data_out13 == data12);
  KASSERT(list3->length == 2);
  KASSERT(list3->first->data == data11);
  KASSERT(list3->last->data == data10);
  KASSERT(key == -2);
  KASSERT(list3->last->key == 0);

  int *data14 = kmalloc(sizeof(int));
  *data14 = INT_MIN;
  linked_list_insert(list3, INT_MAX, data14);

  KASSERT(list3->length == 3);
  KASSERT(list3->last->data == data14);
  KASSERT(list3->last->key == INT_MAX);
  KASSERT(list3->first->data == data11);
  KASSERT(list3->first->key == -1);
  KASSERT(list3->first->prev == NULL);
  KASSERT(list3->first->next != NULL);
  KASSERT(list3->first->next->key == 0);
  KASSERT(list3->first->next->data == data10);
  KASSERT(list3->first->next->prev == list3->first);

  int *data15 = kmalloc(sizeof(int));
  *data15 = INT_MAX;
  linked_list_insert(list3, INT_MIN, data15);

  KASSERT(list3->length == 4);
  KASSERT(list3->last->data == data14);
  KASSERT(list3->last->key == INT_MAX);
  KASSERT(list3->first->data == data15);
  KASSERT(list3->first->key == INT_MIN);
  KASSERT(list3->first->prev == NULL);
  KASSERT(list3->first->next != NULL);
  KASSERT(list3->first->next->key == -1);
  KASSERT(list3->first->next->data == data11);
  KASSERT(list3->first->next->prev != NULL);
  KASSERT(list3->first->next->next != NULL);
  KASSERT(list3->first->next->next->data == data10);
  KASSERT(list3->first->next->next->key == 0);
  KASSERT(list3->first->next->next->next == list3->last);

  void *data_out16 = linked_list_remove_head(list3, &key);
  KASSERT(data_out16 == data15);
  KASSERT(list3->length == 3);
  KASSERT(key == INT_MIN);
  KASSERT(list3->first->data == data11);
  KASSERT(list3->first->key == -1);
  KASSERT(list3->last->data == data14);
  KASSERT(list3->last->key == INT_MAX);

  kprintf("This sentence is to verify that all the custom tests are passed.\n");

  return 0;

}
