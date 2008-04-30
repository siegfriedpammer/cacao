#ifndef _MACHINE_INSTR_H
#define _MACHINE_INSTR_H

static inline long
__attribute__ ((unused))
compare_and_swap (volatile long *p, long oldval, long newval)
{
  long ret;

  __asm__ __volatile__ ("lock; cmpxchgl %2, %1"
                        : "=a" (ret), "=m" (*p)
                        : "r" (newval), "m" (*p), "0" (oldval));
  return ret;
}

#define STORE_ORDER_BARRIER() __asm__ __volatile__ ("" : : : "memory");
#define MEMORY_BARRIER_AFTER_ATOMIC() /* nothing */
#define MEMORY_BARRIER() __asm__ __volatile__ ( \
		"mfence" : : : "memory" );

#endif
