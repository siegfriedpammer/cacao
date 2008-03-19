#ifndef _MACHINE_INSTR_H
#define _MACHINE_INSTR_H

#include "toolbox/logging.h"

static inline long
__attribute__ ((unused))
compare_and_swap (volatile long *p, long oldval, long newval)
{
  long ret;
  /*dolog("compare_and_swap(%p [%d], %d, %d)", p, *p, oldval, newval);*/

  __asm__ __volatile__ (
    "mov %3,%0\n\t"
    "casx [%4],%2,%0\n\t"
    : "=&r"(ret), "=m"(*p) 
    : "r"(oldval), "r"(newval), "r"(p));

  /*dolog("compare_and_swap() return=%d mem=%d", ret, *p);*/
  return ret;
}

#define STORE_ORDER_BARRIER() __asm__ __volatile__ ("wmb" : : : "memory");
#define MEMORY_BARRIER_AFTER_ATOMIC() __asm__ __volatile__ ("mb" : : : "memory");
#define MEMORY_BARRIER() __asm__ __volatile__ ( \
		"membar 0x0F" : : : "memory" );

#endif
