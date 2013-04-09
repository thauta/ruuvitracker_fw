/* 
 * Basic circular buffer implementation. (Ring-buffer)
 * Author: Seppo Takalo
 */

#include "ringbuff.h"
#include <stdlib.h>

/**
 * Allocates new buffer.
 */
struct rbuff *rbuff_new(int size)
{
  struct rbuff *p;
  p = calloc(1, sizeof(struct rbuff));
  if (NULL == p)
    return NULL;
  p->data = malloc(size);
  if (NULL == p->data) {
    free(p);
    return NULL;
  }
  p->size=size;
  return p;
}

/* Deletes previously allocated buffer. */
void rbuff_delete(struct rbuff *p)
{
  if (NULL == p)
    return;
  free(p->data);
  free(p);
}

/* Push one byte to a buffer. */
void rbuff_push(struct rbuff *p, unsigned char c)
{
  if (rbuff_is_full(p)) {
    return;
  }
  p->data[p->top] = c;
  p->top++;
  p->top %= (p->size+1); //Wrap around
}

/* Get one byte from a buffer. Returns 0 is buffer is empty. */
unsigned char rbuff_pop(struct rbuff *p)
{
  unsigned char ret;
  if (rbuff_is_empty(p))
  	return 0;
  ret = p->data[p->bottom++];
  p->bottom %= (p->size+1); //Wrap around
  return ret;

}

/* Test if buffer is empty. Returns 1 on empty, 0 otherwise. */
int rbuff_is_empty(struct rbuff *p)
{
  if (p->top == p->bottom) {
    return 1;
  } else {
    return 0;
  }
}

/* Test if buffer is full. Returns 1 on full, 0 otherwise */
int rbuff_is_full(struct rbuff *p)
{
  if (NULL == p)
    return 1;
  int x = p->top+1;
  x %= (p->size+1);
  if (x == p->bottom) {
    return 1;
  } else {
    return 0;
  }
}

/* Return raw pointer to bottom of buffer */
char *rbuff_get_raw(struct rbuff *p)
{
  if (NULL==p)
    return NULL;
  return (char*) &p->data[p->bottom];
}

/* Remove bytes from bottom of buffer */
void rbuff_remove(struct rbuff *p, int n)
{
  if ( (n<0) || (n>p->size) )
    return;
  for(;n>0;n--)
    rbuff_pop(p);
}
