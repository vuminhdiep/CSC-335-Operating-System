#include <shared_buffer.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>

sb *shared_buffer_create(int sb_size){
    sb *b = kmalloc(sizeof(sb));
    b->counter = b->in = b->out = 0;

    b->BUFFER_SIZE = sb_size;

    b->buffer = kmalloc(sizeof(void*) * (b->BUFFER_SIZE));

    b->lock = kmalloc(sizeof(lock));
    if (b->lock == NULL) {
		return NULL;
	}

    b->lock = lock_create("shared-buffer-lock");
    if (b->lock == NULL) {
		kfree(b->lock);
        kfree(b->buffer);
        kfree(b);
		return NULL;
	}

    b->prod_cv = kmalloc(sizeof(cv));
    if (b->prod_cv == NULL) {
		return NULL;
	}

    b->prod_cv = cv_create("shared-buffer-prod-cv");
    if (b->prod_cv == NULL) {
		kfree(b->prod_cv);
        kfree(b->lock);
        kfree(b->buffer);
        kfree(b);
		return NULL;
	}

    b->con_cv = kmalloc(sizeof(cv));
    if (b->con_cv == NULL) {
		return NULL;
	}

    b->con_cv = cv_create("shared-buffer-con-cv");
    if (b->con_cv == NULL) {
		kfree(b->con_cv);
        kfree(b->prod_cv);
        kfree(b->lock);
        kfree(b->buffer);
        kfree(b);
		return NULL;
	}

    return b;
}


void
shared_buffer_destroy(sb *b){
    KASSERT(b != NULL);
    KASSERT(b->BUFFER_SIZE > 0);
    KASSERT(b->buffer != NULL);
    KASSERT(b->lock != NULL);
    KASSERT(b->prod_cv != NULL);
    KASSERT(b->con_cv != NULL);

    kfree(b->buffer);
    lock_destroy(b->lock);
    cv_destroy(b->prod_cv);
    cv_destroy(b->con_cv);
    kfree(b);
}

void
shared_buffer_produce(sb *b, void *data){
    lock_acquire(b->lock);
    while(b->counter == b->BUFFER_SIZE){
        cv_wait(b->prod_cv, b->lock);
    }
    b->buffer[b->in] = data;
    
    kprintf("Produced item is %d\n", *((int*) data)); //for atomic printing when testing
   
    b->in = (b->in + 1)% b->BUFFER_SIZE;
    b->counter ++;
    cv_signal(b->con_cv, b->lock);
    lock_release(b->lock);
}

void *
shared_buffer_consume(sb *b){
    lock_acquire(b->lock);
    while(b->counter == 0){
        cv_wait(b->con_cv, b->lock);
    }
    void *data = b->buffer[b->out];
    kprintf("Consumed item is %d\n", *((int*)data)); //for atomic printing when testing
    
    b->out = (b->out + 1)% b->BUFFER_SIZE;
    b->counter --;
    cv_signal(b->prod_cv, b->lock);
    lock_release(b->lock);
    return data;
}