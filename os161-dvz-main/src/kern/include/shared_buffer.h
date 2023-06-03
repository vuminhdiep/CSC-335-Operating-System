#ifndef _SHARED_BUFFER_H_
#define _SHARED_BUFFER_H_

#include <types.h>
#include <synch.h>

typedef struct Shared_Buffer Shared_Buffer;
/*
 * The SB structure is also made up of an array of characters, a max
 * size that the buffer can be (the size of the array and max buffer
 * size are determined at the point the buffer is created), and a mutex lock
 * along with two condition variables to make putting characters on and taking 
 * characters off the buffer thread safe.
 * 
 * It also contains a counter to keep track of the amount of items in the buffer,
 * and two integers specifying what the next index into the buffer to read from 
 * and write to is.
 */
struct Shared_Buffer {
  int counter;
  char* buffer;
  int BUFF_SIZE;
  struct lock* buff_lock;
  struct cv* prod_cv;
  struct cv* cons_cv;
  int in;
  int out;
};

/*
 * Creates and allocates space for all components of a shared buffer.
 */
Shared_Buffer *shared_buffer_create(int buf_size);

/*
 * Deconstructs and deallocates a shared buffer.
 */
void shared_buffer_destroy(Shared_Buffer *sb);

/* 
 * Inserts a character at the next available spot in the shared buffer.
 * If the buffer is full this will block until space becomes available.
 */
void
put_char_to_buff(Shared_Buffer *sb, char c);

/* 
 * Takes and returns a character from the next available spot in the shared buffer.
 * If the buffer is empty this will block until another character is produced.
 */
char
get_char_from_buff(Shared_Buffer *sb);

#endif
