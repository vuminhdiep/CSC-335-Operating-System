#include <shared_buffer.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>

int buff_testnum = 1;

/*
 * Creates and allocates space for all components of a shared buffer.
 */
Shared_Buffer *shared_buffer_create(int buf_size){
  Shared_Buffer *buf = kmalloc(sizeof(Shared_Buffer));
  if (buf == NULL){
    return NULL;
  }

  buf->buffer = kmalloc(sizeof(char) * buf_size);
  if (buf->buffer == NULL){
    kfree(buf);
    return NULL;
  }

  buf->buff_lock = lock_create("buffer lock");
  if (buf->buff_lock == NULL){
    kfree(buf->buffer);
    kfree(buf);
    return NULL;
  }

  buf->prod_cv = cv_create("prod cv");
  if (buf->prod_cv == NULL){
    lock_destroy(buf->buff_lock);
    kfree(buf->buffer);
    kfree(buf);
    return NULL;
  }

  buf->cons_cv = cv_create("prod cv");
  if (buf->cons_cv == NULL){
    cv_destroy(buf->prod_cv);
    lock_destroy(buf->buff_lock);
    kfree(buf->buffer);
    kfree(buf);
    return NULL;
  }

  buf->counter = 0;
  buf->BUFF_SIZE = buf_size;
  buf->in = 0;
  buf->out = 0;

  return buf;


}

/*
 * Deconstructs and deallocates a shared buffer.
 */
void shared_buffer_destroy(Shared_Buffer *sb){
  KASSERT(sb != NULL);

  cv_destroy(sb->cons_cv);
  cv_destroy(sb->prod_cv);

  lock_destroy(sb->buff_lock);

  kfree(sb->buffer);
  kfree(sb);
}

/* 
 * Inserts a character at the next available spot in the shared buffer.
 * If the buffer is full this will block until space becomes available.
 */
void
put_char_to_buff(Shared_Buffer *sb, char c){

  KASSERT(sb != NULL);
  lock_acquire(sb->buff_lock);

  while (sb->counter == sb->BUFF_SIZE){
    cv_wait(sb->prod_cv, sb->buff_lock);
  }
  if (buff_testnum == 5){
    thread_yield();
  }

  sb->buffer[sb->in] = c;
  sb->in = (sb->in + 1) % sb->BUFF_SIZE;
  sb->counter++;
  if (buff_testnum == 6){
    thread_yield();
  }
  cv_signal(sb->cons_cv, sb->buff_lock);

  lock_release(sb->buff_lock);
}

/* 
 * Takes and returns a character from the next available spot in the shared buffer.
 * If the buffer is empty this will block until another character is produced.
 */
char
get_char_from_buff(Shared_Buffer *sb){
  KASSERT(sb != NULL);
  lock_acquire(sb->buff_lock);

  while (sb->counter == 0){
    cv_wait(sb->cons_cv, sb->buff_lock);
  }
  char ret = sb->buffer[sb->out];
  if (buff_testnum == 7){
    thread_yield();
  }
  sb->out = (sb->out + 1) % sb->BUFF_SIZE;
  sb->counter--;

  cv_signal(sb->prod_cv, sb->buff_lock);
  lock_release(sb->buff_lock);
  return ret;

}


