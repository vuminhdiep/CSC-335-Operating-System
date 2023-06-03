#include <shared_buffer.h>
#include <lib.h>
#include <thread.h>
#include <test.h>

extern int buff_testnum;
static struct semaphore *donesem = NULL;


static void loop_adder(void* sb, unsigned long how_many){
  if (buff_testnum == 3){
  kprintf("Begin producing!\n"); 
  }
  int i;
  char start = 'a';
  int stop = (int) how_many;
  Shared_Buffer * buff = (Shared_Buffer *) sb;
  for (i = 0; i < stop; i++){
    put_char_to_buff(buff, start + i);
    if (buff_testnum == 3){
    kprintf("Char produced: %c\n", start + i);
    }
  }

  V(donesem);
}

static void loop_consumer(void* sb, unsigned long how_many){
  if (buff_testnum == 3){
  kprintf("Begin consuming!\n");
  }
  int i;
  char collector;
  int stop = (int) how_many;
  Shared_Buffer * buff = (Shared_Buffer *) sb;
  for (i = 0; i < stop; i++){
    collector = get_char_from_buff(buff);
    if (buff_testnum == 3){
    kprintf("Char consumed: %c\n", collector);
    }
  }

  V(donesem);
}

int shared_buff_test_run(int nargs, char **args)
{

  if (nargs == 2) {
    buff_testnum = args[1][0] - '0'; // XXX - Hack - only works for testnum 0 -- 9
  }

  if (buff_testnum == 1){

  kprintf("\nShared Buffer creation/destruction test...\n\n");

  Shared_Buffer * sb_test = shared_buffer_create(7);

  shared_buffer_destroy(sb_test);

  kprintf("SUCCESS\n\n");


  //kfree(list);
  }
  else if (buff_testnum == 2){
    kprintf("\nShared Buffer unthreaded produce and consume test...\n\n");
    Shared_Buffer * sb_test = shared_buffer_create(7);
    char testout1;
    char testout2;
    char testout3;

    kprintf("Chars put on buffer: a, b, c\n\n");
    put_char_to_buff(sb_test, 'a');
    put_char_to_buff(sb_test, 'b');
    put_char_to_buff(sb_test, 'c');

    testout1 = get_char_from_buff(sb_test);
    testout2 = get_char_from_buff(sb_test);
    testout3 = get_char_from_buff(sb_test);

    kprintf("Chars read from buffer: %c, %c, %c\n\n", testout1, testout2, testout3);

    if (testout1 == 'a' && testout2 == 'b' && testout3 == 'c' 
        && sb_test->counter == 0 && sb_test->in == 3
        && sb_test->out == 3){
          kprintf("SUCCESS\n\n");
        }
    else{
          kprintf("FAIL, WRONG VALUE COUNTS OR VALUES IN BUFF\n\n");
        }

    shared_buffer_destroy(sb_test);
  }
  else if (buff_testnum == 3){
    kprintf("Thread produces too much and signals consumer test...\n\n note that because the printing in the test functions is done after the buffer is manipulated the last two characters will appear to be produced and consumed out of order, but they are in the right order. They just switch threads before printing because of how the CVs work.\n\n");
    donesem = sem_create("donesem", 0);
    Shared_Buffer * sb_test = shared_buffer_create(5);
    thread_fork("puton",
	      NULL,
	      loop_adder,
	      sb_test,
	      6);

  thread_fork("takeoff",
	      NULL,
	      loop_consumer,
	      sb_test,
	      6);
    P(donesem);
    P(donesem);
    if (sb_test->counter == 0 && sb_test->in == 1
	&& sb_test->out == 1){
	kprintf("SUCCESS\n");
    }
    else{
	kprintf("FAIL, WRONG VALUES IN BUFF STRUCT\n");
    
    }
  
    sem_destroy(donesem);
    donesem = NULL;
    shared_buffer_destroy(sb_test);
  }
  else if (buff_testnum == 4){
    kprintf("Test with uneven thread access...\n\n");
    donesem = sem_create("donesem", 0);
    Shared_Buffer * sb_test = shared_buffer_create(5);
    thread_fork("puton",
	      NULL,
	      loop_adder,
	      sb_test,
	      3);
    thread_fork("puton2",
	      NULL,
	      loop_adder,
	      sb_test,
	      4);

  thread_fork("takeoff",
	      NULL,
	      loop_consumer,
	      sb_test,
	      12);
  thread_fork("puton3",
	      NULL,
	      loop_adder,
	      sb_test,
	      5);
    P(donesem);
    P(donesem);
    P(donesem);
    P(donesem);
    if (sb_test->counter == 0 && sb_test->in == 2
	&& sb_test->out == 2){
	kprintf("SUCCESS\n");
    }
    else{
	kprintf("FAIL, WRONG VALUES IN BUFF STRUCT\n");
    
    }
  
    sem_destroy(donesem);
    donesem = NULL;
    shared_buffer_destroy(sb_test);
  }
  else if (buff_testnum == 5){
    donesem = sem_create("donesem", 0);
    kprintf("Thread safety test: interleaving pattern 1\n\n");
    Shared_Buffer * sb_test = shared_buffer_create(5);
    thread_fork("puton",
	      NULL,
	      loop_adder,
	      sb_test,
	      6);

  thread_fork("takeoff",
	      NULL,
	      loop_consumer,
	      sb_test,
	      6);

    P(donesem);
    P(donesem);
    if (sb_test->counter == 0 && sb_test->in == 1
	&& sb_test->out == 1){
	kprintf("SUCCESS\n");
    }
    else{
	kprintf("FAIL, WRONG VALUES IN BUFF STRUCT\n");
    
    }
  
    sem_destroy(donesem);
    donesem = NULL;
    shared_buffer_destroy(sb_test);
  }
  else if (buff_testnum == 6){
    donesem = sem_create("donesem", 0);
    kprintf("Thread safety test: interleaving pattern 2\n\n");
    Shared_Buffer * sb_test = shared_buffer_create(5);
    thread_fork("puton",
	      NULL,
	      loop_adder,
	      sb_test,
	      6);

  thread_fork("takeoff",
	      NULL,
	      loop_consumer,
	      sb_test,
	      6);

    P(donesem);
    P(donesem);
    if (sb_test->counter == 0 && sb_test->in == 1
	&& sb_test->out == 1){
	kprintf("SUCCESS\n");
    }
    else{
	kprintf("FAIL, WRONG VALUES IN BUFF STRUCT\n");
    
    }
  
    sem_destroy(donesem);
    donesem = NULL;
    shared_buffer_destroy(sb_test);
  }
  else if (buff_testnum == 7){
    donesem = sem_create("donesem", 0);
    kprintf("Thread safety test: interleaving pattern 3\n\n");
    Shared_Buffer * sb_test = shared_buffer_create(5);
    thread_fork("puton",
	      NULL,
	      loop_adder,
	      sb_test,
	      6);

  thread_fork("takeoff",
	      NULL,
	      loop_consumer,
	      sb_test,
	      6);
    P(donesem);
    P(donesem);
    if (sb_test->counter == 0 && sb_test->in == 1
	&& sb_test->out == 1){
	kprintf("SUCCESS\n");
    }
    else{
	kprintf("FAIL, WRONG VALUES IN BUFF STRUCT\n");
    
    }
  
    sem_destroy(donesem);
    donesem = NULL;
    shared_buffer_destroy(sb_test);
  }
  else {
    kprintf("Not a valid shared buffer test number. \n \n");
  }



  return 0;
}
