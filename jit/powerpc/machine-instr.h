#ifndef _MACHINE_INSTR_H
#define _MACHINE_INSTR_H

static inline long compare_and_swap(long *p, long oldval, long newval)
{
    if (*p == oldval) {
        *p = newval;
        return oldval;
    } else
        return *p;
}

static inline void
atomic_add(int *mem, int val)
{
	*mem += val;
}

#define STORE_ORDER_BARRIER() __asm__ __volatile__ ("" : : : "memory");
#define MEMORY_BARRIER_BEFORE_ATOMIC() __asm__ __volatile__ ("sync" : : : "memory");
#define MEMORY_BARRIER_AFTER_ATOMIC() __asm__ __volatile__ ("" : : : "memory");
#define MEMORY_BARRIER() __asm__ __volatile__ ( "sync" : : : "memory" );

#endif
