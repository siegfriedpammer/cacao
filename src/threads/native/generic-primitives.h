#ifndef _MACHINE_INSTR_H
#define _MACHINE_INSTR_H

#include <pthread.h>

extern pthread_mutex_t _atomic_add_lock;
extern pthread_mutex_t _cas_lock;
extern pthread_mutex_t _mb_lock;


static inline void atomic_add(volatile int *mem, int val)
{
  pthread_mutex_lock(&_atomic_add_lock);

  /* do the atomic add */
  *mem += val;

  pthread_mutex_unlock(&_atomic_add_lock);
}


static inline long compare_and_swap(volatile long *p, long oldval, long newval)
{
  long ret;

  pthread_mutex_lock(&_cas_lock);

  /* do the compare-and-swap */

  ret = *p;

  if (oldval == ret)
    *p = newval;

  pthread_mutex_unlock(&_cas_lock);

  return ret;
}


#define MEMORY_BARRIER()                  (pthread_mutex_lock(&_mb_lock), \
                                           pthread_mutex_unlock(&_mb_lock))
#define STORE_ORDER_BARRIER()             MEMORY_BARRIER()
#define MEMORY_BARRIER_BEFORE_ATOMIC()    /* nothing */
#define MEMORY_BARRIER_AFTER_ATOMIC()     /* nothing */

#endif /* _MACHINE_INSTR_H */
