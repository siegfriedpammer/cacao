/* First Fit FIFO */

#include "allocator3.h"
#include <stdlib.h>

typedef struct cacao_freeblock_t { 
  cacao_size_t               size;
  struct cacao_freeblock_t*  next;
  struct cacao_freeblock_t*  prev; /* now a double-linked list */
} *cacao_freeblock_ptr_t;

static cacao_freeblock_ptr_t first = NULL;
static cacao_freeblock_ptr_t last  = NULL;

void allocator_init()
{
}

void allocator_close()
{
}

void allocator_reset()
{
}

void* allocator_alloc(cacao_size_t size)
{
  /* first fit */
  cacao_freeblock_ptr_t curr = first;

  while (curr) {
    cacao_size_t  curr_size = curr->size & ~0x3;

    if (curr_size > size) {
      /* split */
      cacao_freeblock_ptr_t  next = curr + size;
      cacao_size_t           new_size = curr_size - size;

      if (new_size >= sizeof(cacao_freeblock_ptr_t)) {
	/* free block */
	next->size = new_size | 0x3;
	next->next = curr->next;
	next->prev = curr;
	curr->next = next;
      } else {
	/* small block */
	next->size = curr->size - size | 0x1;
      }
    }

    if (curr->size == size) {
      curr->prev->next = curr->next;
      curr->next->prev = curr->prev;
      return curr;
    }
  }

  return NULL;
}

void allocator_free_block(cacao_ptr_t block_addr, cacao_size_t size)
{
  cacao_freeblock_ptr_t  freeblock = (cacao_freeblock_ptr_t)block_addr;

  /* reject small free blocks */
  if (size < sizeof(struct cacao_freeblock_t))
    return;

  /* store size in freeblock & flag it as free (2 least sign. bits) */
  freeblock->size = size | 0x3;
  /* prepend to list */
  freeblock->next = NULL;
  freeblock->prev = last;
  if (last)
    last->next = freeblock;

  /* append (fifo) */
  last = freeblock;
  if (!first)
    first = last;
}

void allocator_free_prejoin (cacao_ptr_t block_addr, cacao_ptr_t pre_addr, cacao_size_t pre_size)
{
  cacao_freeblock_ptr_t  freeblock = (cacao_freeblock_ptr_t)block_addr;
  cacao_freeblock_ptr_t  preblock = (cacao_freeblock_ptr_t)pre_addr;

  preblock->size = freeblock->size + pre_size;
  preblock->next = freeblock->next;
  preblock->prev = freeblock->prev;
  
  if (preblock->next)
    preblock->next->prev = preblock;
  if (preblock->prev)
    preblock->prev->next = preblock;
}

void allocator_free_postjoin(cacao_ptr_t block_addr, cacao_ptr_t post_addr, cacao_size_t post_size)
{
  cacao_freeblock_ptr_t  freeblock = (cacao_freeblock_ptr_t)block_addr;

  freeblock->size += post_size;
}

void allocator_free_blockjoin (cacao_ptr_t first_addr, cacao_ptr_t last_addr, cacao_size_t extra_size)
{
  cacao_freeblock_ptr_t  freeblock = (cacao_freeblock_ptr_t)first_addr;
  cacao_freeblock_ptr_t  endblock  = (cacao_freeblock_ptr_t)end_addr;
  
  freeblock->size += (endblock->size & ~0x3) + extra_size;

  if (endblock->next)
    endblock->next->prev = endblock->prev;
  if (endblock->prev)
    endblock->prev->next = endblock->next;
}
