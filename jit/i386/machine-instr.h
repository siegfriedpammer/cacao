#ifndef _MACHINE_INSTR_H
#define _MACHINE_INSTR_H

static inline long
__attribute__ ((unused))
atomic_swap (volatile long *mem, long val)
{
  __asm__ __volatile__ ("xchgl %2,%0"
            : "=r" (val) : "0" (val), "m" (*mem));
  return val;
}

static inline long int
__attribute__ ((unused))
compare_and_swap (volatile long int *p, long int oldval, long int newval)
{
  long int ret;

  __asm__ __volatile__ ("lock; cmpxchgl %2, %1"
                        : "=a" (ret), "=m" (*p)
                        : "r" (newval), "m" (*p), "0" (oldval));
  return ret;
}

#endif
