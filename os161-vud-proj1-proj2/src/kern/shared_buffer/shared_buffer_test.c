#include <shared_buffer.h>
#include <lib.h>
#include <thread.h>
#include <spl.h>
#include <test.h>
#include <synch.h>
#include <kern/test161.h>
#include <kern/secret.h>

static struct semaphore *sem = NULL;
static struct sb *testsb = NULL;

#define NLOOPS 10
#define NPRODTHREADS 2
#define NCONTHREADS 2


static
void
producer_thread(void *buffer, unsigned long thread_no)
{
    int i;
    
    testsb = (sb *) buffer;
    (void)thread_no;
   
    for (i = 0; i < NLOOPS; i++){
        int *data = kmalloc(sizeof(int));
        *data = 10 * thread_no + i;
        shared_buffer_produce(testsb, data);
       
        //kprintf_n("Producer at thread %2lu\n", thread_no);
        

    }
    V(sem);
}

static
void
consumer_thread(void *buffer, unsigned long thread_no)
{
    testsb = (sb *) buffer;
    int i;

    (void)thread_no;
    for (i = 0; i < NLOOPS; i++){
        void* data = shared_buffer_consume(testsb);
        //kprintf_n("Consumer at thread %2lu\n", thread_no);
        kfree(data);
    }
    V(sem);
}

int shared_buffer_test(int nargs, char **args)
{
    (void)nargs;
    (void)args;
    int i, result;

    testsb = shared_buffer_create(10);
    sem = sem_create("shared-buffer-prod-sem", 0);
    if (sem == NULL){
        panic("Shared buffer sem_create failed\n");
    }


    for(i = 0; i <NPRODTHREADS; i++){
        result = thread_fork("shared buffer prod thread", NULL, producer_thread, testsb, i);
        if (result){
            panic("shared buffer test: thread_fork failed %s\n", strerror(result));
        }
    }

    for(i = 0; i <NCONTHREADS; i++){
        result = thread_fork("shared buffer con thread", NULL, consumer_thread, testsb, i);
        if (result){
            panic("shared buffer test: thread_fork failed %s\n", strerror(result));
        }
    }

    if (NPRODTHREADS <= NCONTHREADS){
        KASSERT(testsb->counter == 0);
    }


    for(i = 0; i <NPRODTHREADS+NCONTHREADS; i++){
        P(sem);
    }


    sem_destroy(sem);
    sem = NULL;
    shared_buffer_destroy(testsb);

    success(TEST161_SUCCESS, SECRET, "shared buffer test");
    return 0;
}

