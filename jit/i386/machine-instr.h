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

static inline char
__attribute__ ((unused))
compare_and_swap (volatile long int *p, long int oldval, long int newval)
{
  char ret;
  long int readval;

  __asm__ __volatile__ ("lock; cmpxchgl %3, %1; sete %0"
                        : "=q" (ret), "=m" (*p), "=a" (readval)
                        : "r" (newval), "m" (*p), "2" (oldval));
  return ret;
}

#endif
