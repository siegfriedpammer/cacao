#ifndef _MACHINE_INSTR_H
#define _MACHINE_INSTR_H

static inline void
__attribute__ ((unused))
atomic_add (volatile int *mem, int val)
{
#if 0
  __asm__ __volatile__ ("lock; addl %1,%0"
						: "=m" (*mem) 
						: "ir" (val), "m" (*mem));
#endif
}

static inline long
__attribute__ ((unused))
compare_and_swap (volatile long *p, long oldval, long newval)
{
#if 0
  long ret;

  __asm__ __volatile__ ("lock; cmpxchgq %2, %1"
                        : "=a" (ret), "=m" (*p)
                        : "r" (newval), "m" (*p), "0" (oldval));
  return ret;
#endif
}

#if 0
#define STORE_ORDER_BARRIER() __asm__ __volatile__ ("" : : : "memory");
#define MEMORY_BARRIER_BEFORE_ATOMIC() __asm__ __volatile__ ("" : : : "memory");
#define MEMORY_BARRIER_AFTER_ATOMIC() __asm__ __volatile__ ("" : : : "memory");
#define MEMORY_BARRIER() __asm__ __volatile__ ( \
		"mfence" : : : "memory" )
#endif

#define STORE_ORDER_BARRIER()
#define MEMORY_BARRIER_BEFORE_ATOMIC()
#define MEMORY_BARRIER_AFTER_ATOMIC()
#define MEMORY_BARRIER()

#endif
