#include <linked_list.h>
#include <lib.h>
#include <thread.h>
#include <spl.h>
#include <test.h>

extern int testnum;
int cnum = 0;
int testingkeys[8] = {-12, 8, 0, 4, -20, 50, 34, -41};
static struct semaphore *donesem = NULL;

static void linked_list_test_adder(void *list, unsigned long which)
{
  //int s = splhigh(); //added suggested code in spl.h


  int i;
  int *c;

  for (i = 0; i < 4; i++) {
    c = kmalloc(sizeof(int));
    *c = 'A' + i;
    linked_list_prepend(list, c);
   
    linked_list_printlist(list, which);
  }
  Linked_List * lslen = (Linked_List *) list;
  kprintf("length of list: %d\n", lslen -> length);
  //splx(s); // added suggested code in spl.h
  V(donesem);
}



static void linked_list_test_adder_insert(void *list, unsigned long which)
{
  //int s = splhigh(); //added suggested code in spl.h

  int i;
  int *c;
  int letter = 0;

  if (which != 2){
    for (i = 1; i < 8; i = i + 2) {
    c = kmalloc(sizeof(int));
    *c = 'B' + letter;
    linked_list_insert(list, testingkeys[i], c);
    letter = letter + 2;
    linked_list_printlist(list, which);
  }
  }
  else{
    for (i = 0; i < 8; i = i + 2) {
    c = kmalloc(sizeof(int));
    *c = 'A' + letter;
    linked_list_insert(list, testingkeys[i], c);
    letter = letter + 2;
    linked_list_printlist(list, which);
  }

  }
  Linked_List * lslen = (Linked_List *) list;
  kprintf("length of list: %d\n", lslen -> length);
  //splx(s); // added suggested code in spl.h
  V(donesem);
}

static void linked_list_insert_tester(Linked_List *ls, char to_insert, int key){
    int * ch;
    Linked_List_Node * fst;
    Linked_List_Node * lst;

    ch = kmalloc(sizeof(int));
    *ch = to_insert;
    linked_list_insert(ls, key, ch);
    linked_list_printlist(ls, 3);
    fst = ls -> first;
    lst = ls -> last;
    kprintf("first: %d[%c] \n", fst -> key, *((int *)fst -> data));
    kprintf("last: %d[%c] \n", lst -> key, *((int *)lst -> data));
    kprintf("length: %d\n\n", ls -> length);
    //V(donesem);
  }

static void linked_list_remove_tester(void *ls, unsigned long num){
//	(void)dummy;
   // Linked_List_Node * frst;
   // Linked_List_Node * last;
    Linked_List * lst = (Linked_List *)ls;

    int * ksaved = kmalloc(sizeof(int*));
    void *datareturn;
    int i;

    for (i =0; i < 4; i++){
    datareturn = linked_list_remove_head(lst, ksaved);
    linked_list_printlist(lst, (int)num); //condition this too
    kprintf("removed data: %c", *((int*)datareturn));
    kprintf("\nremoved key: %d\n", *((int*)ksaved));
    }
    
    kfree(ksaved);
    kfree(datareturn);
    //kprintf("\nlength: %d\n", lst->length);
   // kprintf("first: %d[%c] \n", lst -> first -> key, *((int *)lst ->first -> data));
   // kprintf("last: %d[%c] \n", lst -> last -> key, *((int *)lst ->last -> data));

    V(donesem);
  }

int linked_list_test_run(int nargs, char **args)
{

  if (nargs == 2) {
    testnum = args[1][0] - '0'; // XXX - Hack - only works for testnum 0 -- 9
  }

  if (testnum == 1){

  donesem = sem_create("donesem", 0);

  kprintf("testnum: %d\n", testnum);

  Linked_List * list = linked_list_create();
  
  thread_fork("adder 1",
	      NULL,
	      linked_list_test_adder,
	      list,
	      1);

  thread_fork("adder 2",
	      NULL,
	      linked_list_test_adder,
	      list,
	      2);
  P(donesem);
  P(donesem);
  
  sem_destroy(donesem);
  donesem = NULL;

  //kfree(list);
  }
  else if (testnum == 2){
    donesem = sem_create("donesem", 0);

    kprintf("testnum: %d\n", testnum);

    Linked_List * list = linked_list_create();
  
  thread_fork("adder 1",
	      NULL,
	      linked_list_test_adder_insert,
	      list,
	      1);

  thread_fork("adder 2",
	      NULL,
	      linked_list_test_adder_insert,
	      list,
	      2);
  P(donesem);
  P(donesem);
  
  sem_destroy(donesem);
  donesem = NULL;
  }
  else if (testnum == 3){
    donesem = sem_create("donesem", 0);
    kprintf("testnum: %d\n", testnum);

    Linked_List * list = linked_list_create();
  
  thread_fork("adder 1",
	      NULL,
	      linked_list_test_adder_insert,
	      list,
	      1);

  thread_fork("adder 2",
	      NULL,
	      linked_list_test_adder_insert,
	      list,
	      2);
  P(donesem);
  P(donesem);
  
  sem_destroy(donesem);
  donesem = NULL;
  }
  else if (testnum == 4){
    donesem = sem_create("donesem", 0);
    kprintf("testnum: %d\n", testnum);

    Linked_List * list = linked_list_create();
    linked_list_insert_tester(list, 'C', 12);
    linked_list_insert_tester(list, 'B', 0);
    linked_list_insert_tester(list, 'A', -42);
    linked_list_insert_tester(list, 'E', 441);
    linked_list_insert_tester(list, 'D', 302);
    linked_list_insert_tester(list, 'F', 442);
    linked_list_insert_tester(list, 'G', 443);
    linked_list_insert_tester(list, 'H', 447);
  
  thread_fork("remover 1",
	      NULL,
	      linked_list_remove_tester,
	      list,
	      1);

  thread_fork("remover 2",
	      NULL,
	      linked_list_remove_tester,
	      list,
	      2);
  P(donesem);
  P(donesem);
  if (list->first == NULL && list->last == NULL){
  kprintf("\n both first and last null...SUCCESS\n");}
  sem_destroy(donesem);
  donesem = NULL;
  }
  else if (testnum == 5){
    donesem = sem_create("donesem", 0);
    kprintf("baseline tests threaded removal:\n\n");

    Linked_List * list = linked_list_create();

    linked_list_insert_tester(list, 'C', 12);
    linked_list_insert_tester(list, 'B', 0);
    linked_list_insert_tester(list, 'A', -42);
    linked_list_insert_tester(list, 'E', 441);
    linked_list_insert_tester(list, 'D', 302);
    linked_list_insert_tester(list, 'F', 442);
    linked_list_insert_tester(list, 'G', 443);
    linked_list_insert_tester(list, 'H', 447);
  
  thread_fork("adder 1",
	      NULL,
	      linked_list_remove_tester,
	      list,
	      0);

  thread_fork("adder 2",
	      NULL,
	      linked_list_remove_tester,
	      list,
	      0);
  P(donesem);
  P(donesem);
  
  sem_destroy(donesem);
  donesem = NULL;
  }
  else if (testnum == 7){
    donesem = sem_create("donesem", 0);
    kprintf("baseline tests threaded insert:\n\n");

    Linked_List * list = linked_list_create();
  
  thread_fork("adder 1",
	      NULL,
	      linked_list_test_adder_insert,
	      list,
	      1);

  thread_fork("adder 2",
	      NULL,
	      linked_list_test_adder_insert,
	      list,
	      2);
  P(donesem);
  P(donesem);
  
  sem_destroy(donesem);
  donesem = NULL;
  }
  else if (testnum == 8){
    int * test = kmalloc(sizeof(int));
    kfree(test);
    //kfree(test);
  }
  else if (testnum == 6){
    kprintf("Non-threaded tests:\n\n");



  //int * ch;
  //Linked_List_Node * fst;
  //Linked_List_Node * lst;

  Linked_List * list2 = linked_list_create();

  kprintf("insertion tests without threads and with many keys: \n");

  linked_list_insert_tester(list2, 'C', 12);
  linked_list_insert_tester(list2, 'B', 0);
  linked_list_insert_tester(list2, 'A', -42);
  linked_list_insert_tester(list2, 'E', 441);
  linked_list_insert_tester(list2, 'D', 302);

  kprintf("\n \n removal tests without threads, removing whole list: \n");

  //linked_list_remove_tester(list2);
  

    linked_list_remove_tester(list2, 0);
    linked_list_remove_tester(list2, 0);
    linked_list_remove_tester(list2, 0);
    linked_list_remove_tester(list2, 0);
    linked_list_remove_tester(list2, 0);

    kprintf("Testing remove on empty list...\n\n");
    int * ksaved = kmalloc(sizeof(int*));
    void *datareturn;

    datareturn = linked_list_remove_head(list2, ksaved);

    if (datareturn == NULL){
      kprintf("Return was NULL: Pass!\n");
    }else {
      kprintf("Return not NULL: Fail!\n");
    }

    kfree(ksaved);
    kfree(datareturn);
  }
  else {
    kprintf("Not a valid linked list test number. \n \n");
  }

  
  

  // XXX - Bug - We're returning from this function without waiting
  // for these two threads to finish.  The execution of these
  // threads may interleave with the kernel's main menu thread and
  // cause interleaving of console output.  We going to accept this
  // problem for the moment until we learn how to fix in Project 2.
  // An enterprising student might investigate why this is not a
  // problem with other tests suites the kernel uses.

  return 0;
}
