#ifndef _MACHINE_INSTR_H
#define _MACHINE_INSTR_H

static inline long
atomic_swap (volatile long *p, long val)
{
  long int result;
  __asm__ __volatile__ ("\n\
0:  lwarx   %0,0,%1 \n\
    stwcx.  %2,0,%1 \n\
    bne-    0b  \n\
" : "=&r"(result) : "r"(p), "r"(val) : "cr0", "memory");
  return result;
}

static inline int
compare_and_swap (volatile long *p, long oldval, long newval)
{
  int result;
  __asm__ __volatile__ ("\n\
0:  lwarx   %0,0,%1 \n\
    sub%I2c.  %0,%0,%2    \n\
    cntlzw  %0,%0   \n\
    bne-    1f  \n\
    stwcx.  %3,0,%1 \n\
    bne-    0b  \n\
1:  \n\
" : "=&b"(result) : "r"(p), "Ir"(oldval), "r"(newval) : "cr0", "memory");
  return result >> 5;
}

#endif
