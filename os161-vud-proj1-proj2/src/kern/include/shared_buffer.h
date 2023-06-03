#ifndef _SHARED_BUFFER_H_
#define _SHARED_BUFFER_H_

#include <types.h>
#include <synch.h>

typedef struct lock lock;
typedef struct cv cv;
typedef struct sb sb;


/**Define a structure encoding a shared buffer of characters.**/
struct sb {
    int counter;
    void **buffer;
    int in;
    int out;
    int BUFFER_SIZE;
    lock *lock;
    cv *prod_cv;
    cv *con_cv;

};

/**Allow the creation of shared buffers of a given length (BUFFER_SIZE)**/
sb *shared_buffer_create(int sb_size);

/**Allow the destruction of shared buffers.**/
void
shared_buffer_destroy(sb *b);

/**Allow characters produced to be added to the buffer.**/
void
shared_buffer_produce(sb *b, void *data);

/**Allow characters to be consumed from the buffer.**/
void *
shared_buffer_consume(sb *b);





#endif